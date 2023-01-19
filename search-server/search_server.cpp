#include "search_server.h"

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status,
                                const std::vector<int>& ratings) {

    if (document_id < 0) {
        throw std::invalid_argument("Negative document ID"s);
    }
    if (documents_.count(document_id)) {
        throw std::invalid_argument("Document with this ID already exists"s);
    }
    if (!CheckSpecialCharInText(document)) {
        throw std::invalid_argument("Special char in document"s);
    }

    const std::vector<std::string_view> words = SplitIntoWordsNoStop(document);
    
    const double inv_word_count = words.empty() ? 0 : 1.0 / words.size();

    for (const std::string_view word : words) {
        auto it = std::find(data_.begin(), data_.end(), word);
        if (it == data_.end()) {
            data_.push_back(static_cast<std::string>(word));
            it = std::next(data_.end(), -1);
        }

        word_to_document_freqs_[*it][document_id] += inv_word_count;
        documents_words_[document_id][*it] += inv_word_count;
    }

    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    documents_id_.insert(document_id);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument
                        (const std::string_view raw_query, int document_id) const {
    if (document_id < 0) {
        throw std::out_of_range("Negative document ID"s);
    }
    if (!documents_.count(document_id)) {
        throw std::out_of_range("Document with this ID not found"s);
    }

    const Query query = ParseQuery(raw_query);

    const auto& it = documents_words_.at(document_id);
    std::vector<std::string_view> matched_words;
    
    if (std::any_of(it.begin(), it.end(), [&query](const auto& element) {
                return query.minus_words.count(element.first);})) {
        return {matched_words, documents_.at(document_id).status};
    }

    matched_words.resize(it.size());

    std::vector<std::string_view> words_in_doc(it.size());
    std::transform(it.begin(), it.end(), words_in_doc.begin(), 
                            [](const auto& element) {return element.first;});

    matched_words.resize(std::set_intersection(words_in_doc.begin(), words_in_doc.end(), 
        query.plus_words.begin(), query.plus_words.end(), matched_words.begin()) - matched_words.begin());
    
    return {matched_words, documents_.at(document_id).status};
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument
        (const std::execution::sequenced_policy &, const std::string_view raw_query, int document_id) const {
    return SearchServer::MatchDocument(raw_query, document_id); 
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument
        (const std::execution::parallel_policy &, const std::string_view raw_query, int document_id) const {
    if (document_id < 0) {
        throw std::out_of_range("Negative document ID"s);
    }
    if (!documents_.count(document_id)) {
        throw std::out_of_range("Document with this ID not found"s);
    }

    const QueryVect query = ParseQuery(std::execution::par, raw_query, false);

    const auto& it = documents_words_.at(document_id);
    std::vector<std::string_view> matched_words;
    
    if (it.end() != std::find_first_of(std::execution::par, it.begin(), it.end(), query.minus_words.begin(), query.minus_words.end(), 
                                       [](const auto& lhs, const std::string_view rhs) {return lhs.first == rhs;})) {
        return {matched_words, documents_.at(document_id).status};
    }

    matched_words.resize(query.plus_words.size());

    matched_words.resize(std::copy_if(std::execution::par, query.plus_words.begin(), query.plus_words.end(), 
                                matched_words.begin(), [&] (const std::string_view word) {return it.count(word);})
                         - matched_words.begin());

    std::sort(std::execution::par, matched_words.begin(), matched_words.end());
    matched_words.erase(std::unique(std::execution::par, matched_words.begin(), matched_words.end()), matched_words.end());
    
    return {matched_words, documents_.at(document_id).status};
}

const std::map<std::string_view, double> SearchServer::GetWordFrequencies (int document_id) const {
    if (documents_words_.count(document_id)) {
        return documents_words_.at(document_id);
    }
    static const std::map<std::string_view, double> empty = {};
    return empty;
}

 void SearchServer::RemoveDocument(int document_id) {
    if (!documents_.count(document_id)) {
        return;
    }

    for (const auto& [word, _] : documents_words_.at(document_id)) {
        word_to_document_freqs_[word].erase(document_id);
    }

    documents_words_.erase(document_id);
    documents_id_.erase(document_id);
    documents_.erase(document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy&, int document_id) {
    SearchServer:: RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy&, int document_id) {
    if (!documents_.count(document_id)) {
        return;
    }

    const auto& it = documents_words_.at(document_id);

    std::vector<std::string_view> words(it.size());

    std::transform(std::execution::par, it.begin(), it.end(), words.begin(), [](const auto& element) {
        return element.first;}); 

    documents_words_.erase(document_id);
    documents_id_.erase(document_id);
    documents_.erase(document_id); 

    std::for_each(std::execution::par, words.begin(), words.end(), [&] (const std::string_view word) {
        word_to_document_freqs_[word].erase(document_id);
        return word;}); 
  
}

bool SearchServer::IsStopWord(const std::string_view word) const {
    return stop_words_.count(word) > 0;
}


std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const {
    std::vector<std::string_view> words;
    for (const std::string_view word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = 0;
    for (const int rating : ratings) {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
        if (!CheckCorrectMinusWord(text)) {
            throw std::invalid_argument("Incorrect word after '-'"s);
        }
    }
    if (text.empty()) {
        throw std::invalid_argument("empty word in request"s);
    }

    if (!CheckSpecialCharInText(text)) {
        throw std::invalid_argument("Special char in search query"s);
    }
    return {text, is_minus, IsStopWord(text)};
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view text) const {
    Query query;
    for (const std::string_view word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.insert(query_word.data);
            } else {
                query.plus_words.insert(query_word.data);
            }
        }
    }
    return query;
}

SearchServer::QueryVect SearchServer::ParseQuery(const std::execution::parallel_policy &, const std::string_view text, const bool is_sort = false) const {
    QueryVect query;
    for (const std::string_view word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.push_back(query_word.data);
            } else {
                query.plus_words.push_back(query_word.data);
            }
        }
    }
    if (is_sort) {
        std::sort(std::execution::par, query.plus_words.begin(), query.plus_words.end());
        query.plus_words.erase(std::unique(std::execution::par, query.plus_words.begin(), query.plus_words.end()), query.plus_words.end());
        
        std::sort(std::execution::par, query.plus_words.begin(), query.plus_words.end());
        query.minus_words.erase(std::unique(std::execution::par, query.minus_words.begin(), query.minus_words.end()), query.minus_words.end());
    }
    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

bool SearchServer::CheckSpecialCharInText(const std::string_view text) const{
    for (char c : text) {
        if ((c <= 31) && (c >=0)) {
            return false;
        }
    }
    return true;
}

bool SearchServer::CheckCorrectMinusWord(const std::string_view text) const {
    if ((text == ""s) || (text[0] == '-') || (text.back() == '-')) {
        return false;
    }
    return true;
}




