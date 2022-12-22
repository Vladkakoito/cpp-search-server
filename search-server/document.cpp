#include "document.h"

std::ostream& operator<< (std::ostream& os, DocumentStatus status) {
    switch (status)
    {
    case DocumentStatus::ACTUAL:
        os << "ACTUAL"s;
        break;
    case DocumentStatus::REMOVED:
        os << "REMOVED"s;
        break;
    case DocumentStatus::IRRELEVANT:
        os << "IRRELEVANT"s;
        break;
    case DocumentStatus::BANNED:
        os << "BANNED"s;
        break;
    }
    return os;
}

std::ostream& operator<< (std::ostream& os, const Document& document) {
    os << "{ document_id = "s << document.id << ", relevance = "s;
    os << document.relevance << ", rating = "s << document.rating << " }"s;
    return os;
}