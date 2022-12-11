

#include <algorithm>
#include <cassert>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <iostream>
#include <numeric>
#include <sstream>
#include <cstdlib>
#include <iomanip>


using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double INACCURACY = 1e-6;

vector<string> SplitIntoWords(const string& text) {

    vector<string> words;
    string word;

    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text) {

        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {

        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();

        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }

        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    }

    template <typename IsActual>
    vector<Document> FindTopDocuments(const string& raw_query, IsActual is_actual) const {

        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, is_actual);

        sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
            if (abs(lhs.relevance - rhs.relevance) < INACCURACY) {
                return lhs.rating > rhs.rating;
            }
            else {
                return lhs.relevance > rhs.relevance;
            }
            });

        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, const DocumentStatus status_request = DocumentStatus::ACTUAL) const {

        return FindTopDocuments(raw_query, [status_request](int document_id, DocumentStatus status, int rating) {
            return status == status_request;
            });
    }

    int GetDocumentCount() const {
        return static_cast<int>(documents_.size());
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {

        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;

        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }

        return { matched_words, documents_.at(document_id).status };
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {

        vector<string> words;

        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }

        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {

        if (ratings.empty()) {
            return 0;
        }

        int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);

        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {

        bool is_minus = false;

        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }

        return { text, is_minus, IsStopWord(text) };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {

        Query query;

        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }

        return query;
    }

    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename IsActual>
    vector<Document> FindAllDocuments(const Query& query, IsActual is_actual) const {

        map<int, double> document_to_relevance;

        for (const string& word : query.plus_words) {

            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }

            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);

            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                if (is_actual(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {

            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }

            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }

        return matched_documents;
    }
};

ostream& operator<<(ostream& os, DocumentStatus status) {

    switch (status) {
    case DocumentStatus::ACTUAL:
        os << "DocumentStatus::ACTUAL"s;
        break;
    case DocumentStatus::IRRELEVANT:
        os << "DocumentStatus::IRRELEVANT"s;
        break;
    case DocumentStatus::BANNED:
        os << "DocumentStatus::BANNED"s;
        break;
    case DocumentStatus::REMOVED:
        os << "DocumentStatus::REMOVED"s;
        break;
    }
    return os;
}

template <typename Type1, typename Type2>
void AssertEqualImpl(const Type1& value1, const Type2 value2, const string& str_value1, const string& str_value2,
    const string& file, const string& func, unsigned line, const string& hint = ""s) {
    
    if (value1 != static_cast<Type1>(value2)) {
        cerr << file << "("s << line << "): "s << func << ": ASSERT_EQUAL("s << str_value1 << ", "s << str_value2;
        cerr << ") failed: "s << value1 << " != "s << value2 << "."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}
#define ASSERT_EQUAL(value1, value2) AssertEqualImpl((value1), (value2), #value1, #value2, __FILE__, __FUNCTION__, __LINE__)
#define ASSERT_EQUAL_HINT(value1, value2, hint) AssertEqualImpl((value1), (value2), #value1, #value2, __FILE__, __FUNCTION__, __LINE__, hint)

void AssertImpl(const bool value, const string& expr, const string& file, const string& func, unsigned line, const string& hint = ""s) {
    if (!value) {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s << "ASSERT("s << expr << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint << endl;;
        }
        abort();
    }
}
#define ASSERT(expr) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__)
#define ASSERT_HINT(expr, hint) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, hint)

template <typename Func>
void RunTestImpl(Func func, const string& func_name) {
    func();
    cerr << func_name << " OK"s << endl;
}
#define RUN_TEST(func) RunTestImpl((func), #func)

// -------- Начало модульных тестов поисковой системы ----------

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT(found_docs.size() == 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop word is ignored"s);
    }
}

void TestExcludeDocumentsContainingMinusWords() {
    const int doc_id = 35;
    const string content = "dog dancing on the table"s;
    const vector<int> ratings = { 9, 2, 6, 1 };

    //добавляем документ, проверяем находится ли он
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("dancing"s);
        ASSERT(found_docs.size() == 1);
    }

    //добавляем минус-слово в запрос, документ не должен находится
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("dancing -table"s).empty(), "Minus word is ignored"s);
    }
}

void TestMatchingDocument() {
    const int doc_id = 100;
    const string content = "little cow swimming near the beach"s;
    const vector<int> ratings = {};

    //проверяем нахождение совпадения слов запроса и заданного документа, без минус слов
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::IRRELEVANT, ratings);
        const auto [matched_words, matched_status] = server.MatchDocument("swimming near"s, doc_id);

        ASSERT_EQUAL(matched_words.size(), 2);
        ASSERT_EQUAL(matched_status, DocumentStatus::IRRELEVANT);
    }

    //проверяем обработку минус слов MatchDocument
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::BANNED, ratings);
        const auto [matched_words, matched_status] = server.MatchDocument("swimming -little"s, doc_id);
        ASSERT_HINT(matched_words.empty(), "Minus word is ignored"s);
    }
}

//проверка сортировки по релевантности
void TestSortFindedDocumentsByRelevance() {
    const int count_docs = 3;
    const vector<int> doc_id = { 6, 1, 4 };
    const vector<string> content = { "big mouse lives on the roof"s,
                                    "strong bee flying near the ear"s,
                                    "happy elephant shakes a tree in front of children"s };
    const vector<vector<int>> ratings = { {4, 2}, 
                                          {1}, 
                                          {9, 10, 9} };

    SearchServer server;
    for (int i = 0; i < count_docs; ++i) {
        server.AddDocument(doc_id[i], content[i], DocumentStatus::ACTUAL, ratings[i]);
    }
    const vector<Document> found_docs = server.FindTopDocuments("mouse bee shakes roof flying big"s);
    for (int i = 1; i < count_docs; ++i) {
        ASSERT_HINT(found_docs[i].relevance < found_docs[i - 1].relevance, "Document N"s + to_string(i) + " after Document N"s + to_string(i-1) + " need"s);
    }
}

//проверка расчета рейтинга
void TestComputngRating() {
    const int doc_id = 43;
    const string content = { "nice snake stand upright"s };
    const vector<int> rating = { 4, 5, 10, 1 };

    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, rating);
    const auto found_docs = server.FindTopDocuments("snake"s);
    ASSERT_EQUAL(found_docs[0].rating, 5);
}


//проверка фильтра по заданной функции
void TestFilterPredicate() {
    const int doc_count = 3;
    const vector<int> doc_ids = { 3, 5, 7 };
    const vector<string> contents = { "white horse jumping to very hight hill with smile on face"s,
                                     "big monkey runing from banan"s,
                                     "small krokodile falling to water"s };
    const vector<vector<int>> ratings = { {4, 2, 1, 5},
                                          {4, 6, 3, 7},
                                          {1, 5, 1, 2} };
    const vector<DocumentStatus> status_input = { DocumentStatus::REMOVED, DocumentStatus::ACTUAL, DocumentStatus::REMOVED };
    const DocumentStatus status_request = DocumentStatus::REMOVED;

    SearchServer server;
    for (int i = 0; i < doc_count; ++i) {
        server.AddDocument(doc_ids[i], contents[i], status_input[i], ratings[i]);
    }

    //статус переносим в функцию, а рейтинг и ID проверяем на равенство. такой документ только один
    const auto found_docs = server.FindTopDocuments("white big water"s, [status_request](const int doc_id, const DocumentStatus status, const int rating) {
        if (rating == doc_id && status == status_request) return true;
        return false;
        });
    ASSERT(found_docs.size() == 1);
    ASSERT_EQUAL_HINT(found_docs[0].id, found_docs[0].rating, "Predicate filter must be find is one document in the test example with ID == rating"s);
}

//тест фильтра по заданному статусу
void TestFilterFromStatus() {
    const int doc_count = 3;
    const vector<int> doc_ids = { 6, 7, 8 };
    const vector<string> contents = { "long man sit on the stone"s,
                                     "rabbit go home"s,
                                     "dog screaming"s };
    const vector<DocumentStatus> status_input = { DocumentStatus::ACTUAL, DocumentStatus::IRRELEVANT, DocumentStatus::BANNED };
    const vector<vector<int>> ratings = { {2, 5, 1},
                                          {1},
                                          {} };
    const DocumentStatus status_request = DocumentStatus::IRRELEVANT;

    SearchServer server;
    for (int i = 0; i < doc_count; ++i) {
        server.AddDocument(doc_ids.at(i), contents.at(i), status_input.at(i), ratings.at(i));
    }

    const auto found_docs = server.FindTopDocuments("long home dog"s, status_request);
    ASSERT_EQUAL_HINT(found_docs.size(), 1, "Is one document with needed status"s);
}

//проверка расчета релевантности
void TestComputeRelevance() {
    const int doc_count = 4;
    const vector<int> doc_ids = { 0, 1, 2, 3 };
    const vector<string> contents = { "белый кот и модный ошейник"s,
                                      "пушистый кот пушистый хвост"s,
                                      "ухоженный пёс выразительные глаза"s,
                                      "ухоженный скворец евгений"s};
    const vector<vector<int>> ratings = { {4, 2, 1},
                                          {1},
                                          {},
                                          {5, 8, 3, 9} };
    const string stop_words = { "и в на"s };
    const vector<double> request_relevance = { 0.866434, 0.231049, 0.173287, 0.173287 };
    const string query = { "пушистый ухоженный кот"s };

    SearchServer server;
    server.SetStopWords(stop_words);
    for (int i = 0; i < doc_count; ++i) {
        server.AddDocument(doc_ids[i], contents[i], DocumentStatus::ACTUAL, ratings[i]);
    }

    const auto found_docs = server.FindTopDocuments(query);
    ASSERT_EQUAL(found_docs.size(), 4);

    ASSERT_HINT(abs(found_docs.at(0).relevance - request_relevance[0]) < INACCURACY, 
        "doc0 relevance: "s + to_string(found_docs.at(0).relevance) + ". Need: "s + to_string(request_relevance[0]));

    ASSERT_HINT(abs(found_docs.at(1).relevance - request_relevance[1]) < INACCURACY,
        "doc1 relevance: "s + to_string(found_docs.at(1).relevance) + ". Need: "s + to_string(request_relevance[1]));

    ASSERT_HINT(abs(found_docs.at(2).relevance - request_relevance[2]) < INACCURACY,
        "doc2 relevance: "s + to_string(found_docs.at(2).relevance) + ". Need: "s + to_string(request_relevance[2]));

    ASSERT_HINT(abs(found_docs.at(3).relevance - request_relevance[3]) < INACCURACY,
        "doc3 relevance: "s + to_string(found_docs.at(3).relevance) + ". Need: "s + to_string(request_relevance[3]));
}


// Функция TestSearchServer является точкой входа для запуска тестов
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

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    cout << "Search server testing finished"s << endl;
}