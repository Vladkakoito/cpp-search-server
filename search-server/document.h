#pragma once

#include <iostream>

using namespace std::string_literals;

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

struct Document {
    Document() = default;

    Document(int id, double relevance, int rating)
        : id(id)
        , relevance(relevance)
        , rating(rating) {
    }

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

std::ostream& operator<< (std::ostream& os, DocumentStatus status);

std::ostream& operator<< (std::ostream& os, const Document& document);