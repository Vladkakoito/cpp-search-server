#include "log_duration.h"

void MatchDocument(SearchServer& server, std::string raw_query, int document_id) {

    std::cout << "Матчинг документа по запросу: "s << raw_query << std::endl;
    LOG_DURATION_STREAM("Operation time"s, std::cout);

    std::vector<std::string> result;
    DocumentStatus status;
    std::tie(result, status) = server.MatchDocument(raw_query, document_id);
    std::cout << "{ document_id = "s << document_id << ", status = "s << status << ", words ="s;
    for (const std::string& word : result) {
        std::cout << " ["s << word << "]"s;
    }
    std::cout << " }"s << std::endl;
}

void FindTopDocuments(SearchServer& server, std::string raw_query) {

    std::cout << "Результаты поиска по запросу: "s << raw_query << std::endl;
    LOG_DURATION_STREAM("Operation time"s, std::cout);

    std::vector<Document> result = server.FindTopDocuments(raw_query);
    for (Document doc : result) {
        std::cout << doc << std::endl;
    }
}