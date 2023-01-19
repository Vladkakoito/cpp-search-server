#pragma once
#include <vector>
#include <string>
#include <set>
#include <stdexcept>
#include <algorithm>

std::vector<std::string_view> SplitIntoWords(const std::string_view text);


template <typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string, std::less<>> non_empty_strings;
    for (const std::string_view str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(static_cast<std::string> (str));
        }
    }
    return non_empty_strings;
}