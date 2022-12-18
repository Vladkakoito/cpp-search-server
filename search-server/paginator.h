#pragma once
#include<vector>
#include <iostream>

#include "document.h"

using namespace std::string_literals;


template<typename Iterator>
struct IteratorRange {
    explicit IteratorRange(Iterator begin, Iterator end)
    :begin_(begin), end_(end) {}

    Iterator begin() const {
        return begin_;
    }
    Iterator end() const {
        return end_;
    }
    size_t size() const {
        return distance(begin_, end_);
    }

private:
    Iterator begin_;
    Iterator end_;
};

std::ostream& operator<< (std::ostream& os, Document element) {
    os << "{ document_id = "s << element.id << ", relevance = "s << element.relevance;
    os << ", rating = "s << element.rating << " }"s;
    return os;
}

template<typename Iterator>
std::ostream& operator<< (std::ostream& os, const IteratorRange<Iterator>& page) {
    for (auto i = page.begin(); i != page.end(); advance(i, 1)) {
        os << *i;
    }
    return os;
}

template <typename Iterator>
class Paginator {
public:
    explicit Paginator (Iterator start, Iterator stop, int page_size) 
    {
        for (Iterator i = start; i != stop;) {
            Iterator page_begin = i;

            if (distance(i, stop) <= page_size) {
                i = stop;
            } else {
                advance(i, page_size);
            }

            pages_.push_back(IteratorRange<Iterator>(page_begin, i));
        }
    }
    auto begin() const {
        return pages_.begin();
    }

    auto end() const {
        return pages_.end();
    }
    size_t size() const {
        return pages_.size();
    }
private:
    std::vector<IteratorRange<Iterator>> pages_;
};

template <typename Container>
auto Paginate (const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}