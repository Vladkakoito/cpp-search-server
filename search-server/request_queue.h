#pragma once
#include <vector>
#include <stack>

#include "search_server.h"

class RequestQueue {
public:
    RequestQueue() = default;
    explicit RequestQueue(const SearchServer& search_server)
    :server_(search_server){}

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, 
                                                        DocumentStatus status = DocumentStatus::ACTUAL) {
            return AddFindRequest(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                                                        return document_status == status;});
    }

    int GetNoResultRequests() const {
        return empty_queries_;
    }

private:
    struct QueryResult {
        uint64_t query_time;
        int result_count;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& server_;
    int empty_queries_ = 0;
    uint64_t current_time_ = 0;
}; 

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    ++current_time_;
    while (!requests_.empty() && (current_time_ - requests_.front().query_time >= min_in_day_)) {
        if (requests_.front().result_count == 0) {
            --empty_queries_;
        }
        requests_.pop_front();
    }
    auto result = server_.FindTopDocuments(raw_query, document_predicate);
    requests_.push_back({current_time_, static_cast<int>(result.size())});
    if (result.empty()) {
        ++empty_queries_;
    }
    return result;
}
