#pragma once

#include <iostream>

#include "search_server.h"

template <typename Type1, typename Type2>
void AssertEqualImpl(const Type1& value1, const Type2 value2, const std::string& str_value1, const std::string& str_value2,
    const std::string& file, const std::string& func, unsigned line, const std::string& hint = ""s) {
    
    if (value1 != static_cast<Type1>(value2)) {
        std::cerr << file << "("s << line << "): "s << func << ": ASSERT_EQUAL("s << str_value1 << ", "s << str_value2;
        std::cerr << ") failed: "s << value1 << " != "s << value2 << "."s;
        if (!hint.empty()) {
            std::cerr << " Hint: "s << hint;
        }
        std::cerr << std::endl;
        abort();
    }
}
#define ASSERT_EQUAL(value1, value2) AssertEqualImpl((value1), (value2), #value1, #value2, __FILE__, __FUNCTION__, __LINE__)
#define ASSERT_EQUAL_HINT(value1, value2, hint) AssertEqualImpl((value1), (value2), #value1, #value2, __FILE__, __FUNCTION__, __LINE__, hint)

void AssertImpl(const bool value, const std::string& expr, const std::string& file, const std::string& func, 
                                                                unsigned line, const std::string& hint = ""s);

#define ASSERT(expr) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__)
#define ASSERT_HINT(expr, hint) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, hint)

template <typename Func>
void RunTestImpl(Func func, const std::string& func_name) {
    func();
    std::cerr << func_name << " OK"s << std::endl;
}
#define RUN_TEST(func) RunTestImpl((func), #func)


void TestExcludeStopWordsFromAddedDocumentContent();

void TestExcludeDocumentsContainingMinusWords();

void TestMatchingDocument();

void TestSortFindedDocumentsByRelevance();

void TestComputngRating();

void TestFilterPredicate();

void TestFilterFromStatus();

void TestComputeRelevance();

void TestSearchServer();
