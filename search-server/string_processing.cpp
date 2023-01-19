#include "string_processing.h"

using namespace std::string_literals;

std::vector<std::string_view> SplitIntoWords(const std::string_view text) {
    std::vector<std::string_view> words;
    auto it = text.begin();

    if (*it == ' ') {
        it = std::find_if(text.begin(), text.end(), [](const char element) {return element != ' ';});
    }

    if (it == text.end()) {
        return words;
    }

    while (it != text.end()) {
        auto it_end = std::adjacent_find(it, text.end(), [] (const char lhs, const char rhs) {
            return lhs != ' ' && rhs == ' ';});

        it_end = it_end == text.end() ? it_end : std::next(it_end);
        std::string_view word = text.substr(std::distance(text.begin(), it), std::distance(it, it_end));

        words.push_back(word);
        
        it = std::find_if(it_end , text.end(), [](const char element) {return element != ' ';});
    }

    return words;
}

