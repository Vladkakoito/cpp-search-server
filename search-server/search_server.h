#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include <tuple>
#include <map>
#include <set>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <execution>
#include <string_view>
#include <list>
#include <future>
#include <type_traits>
#include <cassert>

#include "string_processing.h"
#include "document.h"
#include "log_duration.h"
#include "concurrent_map.h"

using namespace std::string_literals;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double INNACURATE = 1e-6;
const int CONCURRENT_MAP_BUCKETS_COUNT = 500;
const int PARALLEL_PROCESS_COUNT = 11;

class SearchServer
{
public:

    explicit SearchServer(const std::string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text)) {}

    explicit SearchServer(const std::string_view stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text)) {}

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);

    void AddDocument(int document_id, const std::string_view document, DocumentStatus status,
                     const std::vector<int> &ratings);

    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, const std::string_view raw_query, 
                                                                DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const {
        return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
    }

    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, const std::string_view raw_query,
                                           DocumentStatus status = DocumentStatus::ACTUAL) const {
        return FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating)
                                { return document_status == status; });
    }

    std::vector<Document> FindTopDocuments(const std::string_view raw_query,
                                           DocumentStatus status = DocumentStatus::ACTUAL) const {
        return FindTopDocuments(std::execution::seq, raw_query, status);
    }

    int GetDocumentCount() const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument
        (const std::string_view raw_query, int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument
        (const std::execution::sequenced_policy &, const std::string_view raw_query, int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument
        (const std::execution::parallel_policy &, const std::string_view raw_query, int document_id) const;

    auto begin() const
    {
        return documents_id_.begin();
    }

    auto end() const
    {
        return documents_id_.end();
    }

    const std::map<std::string_view, double> GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);

    void RemoveDocument(const std::execution::sequenced_policy &, int document_id);

    void RemoveDocument(const std::execution::parallel_policy &, int document_id);

private:
    struct DocumentData
    {
        int rating;
        DocumentStatus status;
    };
    const std::set<std::string, std::less<>> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> documents_id_;
    std::map<int, std::map<std::string_view, double>> documents_words_;
    std::list<std::string> data_;

    bool IsStopWord(const std::string_view word) const;

    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int> &ratings);

    struct QueryWord
    {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string_view text) const;

    struct Query {
        std::set<std::string_view, std::less<>> plus_words;
        std::set<std::string_view, std::less<>> minus_words;
    };

    struct QueryVect {
        std::vector<std::string_view>plus_words;
        std::vector<std::string_view>minus_words;
    };
    

    Query ParseQuery(const std::string_view text) const;

    QueryVect ParseQuery(const std::execution::parallel_policy &, const std::string_view text, const bool is_sort) const;

    double ComputeWordInverseDocumentFreq(const std::string_view word) const;

    template <typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> FindAllDocuments(ExecutionPolicy&& policy, const QueryVect &query, DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query &query, DocumentPredicate document_predicate) const;

    bool CheckSpecialCharInText(const std::string_view text) const;

    bool CheckCorrectMinusWord(const std::string_view text) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
        
    for (const std::string_view word : stop_words_) {
        if (!CheckSpecialCharInText(word)) {
            throw std::invalid_argument("Special char in stop word"s);
        }
    }
}

template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, const std::string_view raw_query,
                                                     DocumentPredicate document_predicate) const {

    std::vector<Document> result;

   if (std::is_same_v<std::decay_t<ExecutionPolicy>, std::execution::parallel_policy>) {
        const QueryVect query = ParseQuery(std::execution::par, raw_query, true);
        result = FindAllDocuments(policy, query, document_predicate);

   } else {
        const Query query = ParseQuery(raw_query);
        result = FindAllDocuments(query, document_predicate);
   }

    std::sort(policy, result.begin(), result.end(),
            [](const Document &lhs, const Document &rhs){
                if (std::abs(lhs.relevance - rhs.relevance) < INNACURATE) {
                    return lhs.rating > rhs.rating;
                }
                else {
                    return lhs.relevance > rhs.relevance;
                }
            });

    if (result.size() > MAX_RESULT_DOCUMENT_COUNT) {
        result.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return result;
}

template <typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindAllDocuments(ExecutionPolicy&& policy, 
                                                     const QueryVect &query, DocumentPredicate document_predicate) const {

 
    ConcurrentMap<int, double> document_to_relevance(CONCURRENT_MAP_BUCKETS_COUNT);

    auto counter = [&] (std::string_view word) {
    
        if (word_to_document_freqs_.count(word) == 0) {
            return;
        }
        
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        auto& id_word = word_to_document_freqs_.at(word);

        for_each(std::execution::par, id_word.begin(), id_word.end(), [&](const auto element) {
            const auto& document_data = documents_.at(element.first);
            if (document_predicate(element.first, document_data.status, document_data.rating)) {
                document_to_relevance[element.first].ref_to_value += element.second * inverse_document_freq;
            }});
    };

    auto eraser = [&] (std::string_view word) {
        for (const std::string_view word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                return;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.Erase(document_id);
            }
        }
    };
    
    std::for_each(std::execution::par, query.plus_words.begin(), query.plus_words.end(), counter);

    std::for_each(std::execution::par, query.minus_words.begin(), query.minus_words.end(), eraser);
    
    std::map<int, double> document_to_relevance_result = move(document_to_relevance.BuildOrdinaryMap());

    std::vector<Document> matched_documents(document_to_relevance_result.size());
    std::transform(std::execution::par, document_to_relevance_result.begin(), document_to_relevance_result.end(), matched_documents.begin(), 
                [&] (const auto& element) {
                    return Document{element.first, element.second, documents_.at(element.first).rating};});

    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query &query, DocumentPredicate document_predicate) const {

    std::map<int, double> document_to_relevance;
    for (const std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto &document_data = documents_.at(document_id);

            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
}