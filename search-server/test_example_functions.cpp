#include "test_example_functions.h"

using namespace std::string_literals;

const double INACCURACY = 1e-6;

void AssertImpl(const bool value, const std::string& expr, const std::string& file, const std::string& func, 
                unsigned line, const std::string& hint) {
    if (!value) {
        std::cerr << std::boolalpha;
        std::cerr << file << "("s << line << "): "s << func << ": "s << "ASSERT("s << expr << ") failed."s;
        if (!hint.empty()) {
            std::cerr << " Hint: "s << hint << std::endl;;
        }
        abort();
    }
}

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = { 1, 2, 3 };

    {
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        const auto found_docs_par = server.FindTopDocuments(std::execution::par, "in"s);

        ASSERT(found_docs.size() == 1);
        const Document& doc0 = found_docs[0];
        
        ASSERT_EQUAL(doc0.id, doc_id);

        ASSERT_HINT(found_docs[0].id == found_docs_par[0].id, "Incorrect parallel method"s);

    }

    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop word is ignored"s);
        ASSERT_HINT(server.FindTopDocuments(std::execution::par, "in"s).empty(), "Stop word is ignored in parallel version"s);
    }
}

void TestExcludeDocumentsContainingMinusWords() {
    const int doc_id = 35;
    const std::string content = "dog dancing on the table"s;
    const std::vector<int> ratings = { 9, 2, 6, 1 };

    {
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("dancing"s);
        const auto found_docs_par = server.FindTopDocuments(std::execution::par, "dancing"s);

        ASSERT(found_docs.size() == 1);
        ASSERT(found_docs_par.size() == 1);
    }

    {
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

        ASSERT_HINT(server.FindTopDocuments("dancing -table"s).empty(), "Minus word is ignored"s);
        ASSERT_HINT(server.FindTopDocuments(std::execution::par, "dancing -table"s).empty(), "Minus word is ignored in parallel version"s);
    }
}

void TestMatchingDocument() {
    const int doc_id = 100;
    const std::string content = "little cow swimming near the beach"s;
    const std::vector<int> ratings = {};

    {
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::IRRELEVANT, ratings);
        const auto [matched_words, matched_status] = server.MatchDocument("swimming near"s, doc_id);
        const auto [matched_words_par, matched_status_par] = server.MatchDocument(std::execution::par, "swimming near"s, doc_id);

        ASSERT_EQUAL(matched_words.size(), 2);
        ASSERT_EQUAL(matched_status, DocumentStatus::IRRELEVANT);

        ASSERT_EQUAL_HINT(matched_words_par.size(), 2, "Incorrect parallel MatchDocument"s);
        ASSERT_EQUAL_HINT(matched_status_par, DocumentStatus::IRRELEVANT, "Incorrect parallel MatchDocument"s);
    }

    {
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::BANNED, ratings);
        const auto [matched_words, matched_status] = server.MatchDocument("swimming -little"s, doc_id);
        const auto [matched_words_par, matched_status_par] = server.MatchDocument(std::execution::par, "swimming -little"s, doc_id);

        ASSERT_HINT(matched_words.empty(), "Minus word is ignored"s);
        ASSERT_HINT(matched_words_par.empty(), "Minus word is ignored in parallel MatchDocument"s);
    }
}

void TestSortFindedDocumentsByRelevance() {
    const int count_docs = 3;
    const std::vector<int> doc_id = { 6, 1, 4 };
    const std::vector<std::string> content = { "big mouse lives on the roof"s,
                                    "strong bee flying near the ear"s,
                                    "happy elephant shakes a tree in front of children"s };

    const std::vector<std::vector<int>> ratings = { {4, 2}, 
                                          {1}, 
                                          {9, 10, 9} };

    SearchServer server(""s);
    for (int i = 0; i < count_docs; ++i) {
        server.AddDocument(doc_id[i], content[i], DocumentStatus::ACTUAL, ratings[i]);
    }
    const std::vector<Document> found_docs = server.FindTopDocuments("mouse bee shakes roof flying big"s);
    const std::vector<Document> found_docs_par = server.FindTopDocuments(std::execution::par, "mouse bee shakes roof flying big"s);
    for (int i = 1; i < count_docs; ++i) {
        ASSERT_HINT(found_docs[i].relevance < found_docs[i - 1].relevance, "Document N"s + std::to_string(i) + 
                                                        " after Document N"s + std::to_string(i-1) + " need"s); 
        ASSERT_HINT(found_docs_par[i].relevance < found_docs_par[i - 1].relevance, "Par version: Document N"s + std::to_string(i) + 
                                                        " after Document N"s + std::to_string(i-1) + " need"s);
    }
}

void TestComputngRating() {
    const int doc_id = 43;
    const std::string content = { "nice snake stand upright"s };
    const std::vector<int> rating = { 4, 5, 10, 1 };

    SearchServer server(""s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, rating);

    const auto found_docs = server.FindTopDocuments("snake"s);
    const auto found_docs_par = server.FindTopDocuments(std::execution::par, "snake"s);

    ASSERT_EQUAL(found_docs[0].rating, 5);
    ASSERT_EQUAL(found_docs_par[0].rating, 5);
}


void TestFilterPredicate() {
    const int doc_count = 3;
    const std::vector<int> doc_ids = { 3, 5, 7 };
    const std::vector<std::string> contents = { "white horse jumping to very hight hill with smile on face"s,
                                     "big monkey runing from banan"s,
                                     "small krokodile falling to water"s };

    const std::vector<std::vector<int>> ratings = { {4, 2, 1, 5},
                                          {4, 6, 3, 7},
                                          {1, 5, 1, 2} };

    const std::vector<DocumentStatus> status_input = { DocumentStatus::REMOVED, DocumentStatus::ACTUAL, DocumentStatus::REMOVED };
    const DocumentStatus status_request = DocumentStatus::REMOVED;

    SearchServer server(""s);
    for (int i = 0; i < doc_count; ++i) {
        server.AddDocument(doc_ids[i], contents[i], status_input[i], ratings[i]);
    }

    const auto found_docs = server.FindTopDocuments("white big water"s, 
                            [status_request](const int doc_id, const DocumentStatus status, const int rating) {
                                if (rating == doc_id && status == status_request) return true;
                                return false;});

    const auto found_docs_par = server.FindTopDocuments(std::execution::par, "white big water"s, 
                            [status_request](const int doc_id, const DocumentStatus status, const int rating) {
                                if (rating == doc_id && status == status_request) return true;
                                return false;});

    ASSERT(found_docs.size() == 1);
    ASSERT_EQUAL_HINT(found_docs[0].id, found_docs[0].rating, 
        "Predicate filter must be find is one document in the test example with ID == rating"s);

    ASSERT(found_docs_par.size() == 1);
    ASSERT_EQUAL_HINT(found_docs_par[0].id, found_docs_par[0].rating, 
        "In Par version: Predicate filter must be find is one document in the test example with ID == rating"s);
}


void TestFilterFromStatus() {
    const int doc_count = 3;
    const std::vector<int> doc_ids = { 6, 7, 8 };
    const std::vector<std::string> contents = { "long man sit on the stone"s,
                                     "rabbit go home"s,
                                     "dog screaming"s };

    const std::vector<DocumentStatus> status_input = { DocumentStatus::ACTUAL, DocumentStatus::IRRELEVANT, DocumentStatus::BANNED };
    const std::vector<std::vector<int>> ratings = { {2, 5, 1},
                                          {1},
                                          {} };

    const DocumentStatus status_request = DocumentStatus::IRRELEVANT;

    SearchServer server(""s);
    for (int i = 0; i < doc_count; ++i) {
        server.AddDocument(doc_ids.at(i), contents.at(i), status_input.at(i), ratings.at(i));
    }

    const auto found_docs = server.FindTopDocuments("long home dog"s, status_request);
    const auto found_docs_par = server.FindTopDocuments(std::execution::par, "long home dog"s, status_request);

    ASSERT_EQUAL_HINT(found_docs.size(), 1, "Is one document with needed status"s);
    ASSERT_EQUAL_HINT(found_docs_par.size(), 1, "In Par version: Is one document with needed status"s);

    ASSERT_EQUAL_HINT(found_docs[0].id, 7, "Incorrect status in finded document"s);
    ASSERT_EQUAL_HINT(found_docs_par[0].id, 7, "In Par version: Incorrect status in finded document"s);
}

void TestComputeRelevance() {
    const int doc_count = 4;
    const std::vector<int> doc_ids = { 0, 1, 2, 3 };
    const std::vector<std::string> contents = { "белый кот и модный ошейник"s,
                                      "пушистый кот пушистый хвост"s,
                                      "ухоженный пёс выразительные глаза"s,
                                      "ухоженный скворец евгений"s};

    const std::vector<std::vector<int>> ratings = { {4, 2, 1},
                                          {1},
                                          {},
                                          {5, 8, 3, 9} };

    const std::string stop_words = { "и в на"s };
    const std::vector<double> request_relevance = { 0.866434, 0.231049, 0.173287, 0.173287 };
    const std::string query = { "пушистый ухоженный кот"s };

    SearchServer server(stop_words);
    for (int i = 0; i < doc_count; ++i) {
        server.AddDocument(doc_ids[i], contents[i], DocumentStatus::ACTUAL, ratings[i]);
    }

    const auto found_docs = server.FindTopDocuments(query);
    const auto found_docs_par = server.FindTopDocuments(std::execution::par, query);

    ASSERT_EQUAL(found_docs.size(), 4);

    ASSERT_HINT(std::abs(found_docs.at(0).relevance - request_relevance[0]) < INACCURACY, 
        "doc0 relevance: "s + std::to_string(found_docs.at(0).relevance) + ". Need: "s + std::to_string(request_relevance[0]));

    ASSERT_HINT(std::abs(found_docs.at(1).relevance - request_relevance[1]) < INACCURACY,
        "doc1 relevance: "s + std::to_string(found_docs.at(1).relevance) + ". Need: "s + std::to_string(request_relevance[1]));

    ASSERT_HINT(std::abs(found_docs.at(2).relevance - request_relevance[2]) < INACCURACY,
        "doc2 relevance: "s + std::to_string(found_docs.at(2).relevance) + ". Need: "s + std::to_string(request_relevance[2]));

    ASSERT_HINT(std::abs(found_docs.at(3).relevance - request_relevance[3]) < INACCURACY,
        "doc3 relevance: "s + std::to_string(found_docs.at(3).relevance) + ". Need: "s + std::to_string(request_relevance[3]));

    for (int i = 0; i < 4; ++i) {
        ASSERT_HINT((found_docs_par[i].id == found_docs[i].id) && 
                    (found_docs_par[i].relevance == found_docs[i].relevance), "Incorrect par version"s);
    }
}

void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludeDocumentsContainingMinusWords);
    RUN_TEST(TestMatchingDocument);
    RUN_TEST(TestSortFindedDocumentsByRelevance);
    RUN_TEST(TestComputngRating);
    RUN_TEST(TestFilterPredicate);
    RUN_TEST(TestFilterFromStatus);
    RUN_TEST(TestComputeRelevance);
}
