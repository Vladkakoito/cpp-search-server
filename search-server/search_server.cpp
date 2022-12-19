#include "search_server.h"

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status,
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

    const std::vector<std::string> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    id_by_addition_.push_back(document_id);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {

    if (document_id < 0) {
        throw std::invalid_argument("Negative document ID"s);
    }
    if (!documents_.count(document_id)) {
        throw std::invalid_argument("Document with this ID not found"s);
    }

    const Query query = ParseQuery(raw_query);
    std::vector<std::string> matched_words;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    return {matched_words, documents_.at(document_id).status};
} 

int SearchServer::GetDocumentId(int index) const {
    if (index < 0) {
        throw std::out_of_range("Negative index"s);
    }
    if (index > static_cast<int>(id_by_addition_.size())) {
        throw std::out_of_range("Index exceeds the number of documents"s);
    }
    return id_by_addition_[index];
}

bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
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

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const {
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
        if (!CheckCorrectMinusWord(text)) {
            throw std::invalid_argument("Incorrect word after '-'"s);
        }
    }
    if (!CheckSpecialCharInText(text)) {
        throw std::invalid_argument("Special char in search query"s);
    }
    return {text, is_minus, IsStopWord(text)};
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    Query query;
    for (const std::string& word : SplitIntoWords(text)) {
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

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

bool SearchServer::CheckSpecialCharInText(const std::string& text) const{
    for (char c : text) {
        if ((c <= 31) && (c >=0)) {
            return false;
        }
    }
    return true;
}

bool SearchServer::CheckCorrectMinusWord(const std::string& text) const {
    if ((text == ""s) || (text[0] == '-') || (text.back() == '-')) {
        return false;
    }
    return true;
}