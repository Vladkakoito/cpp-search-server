#pragma once

#include <chrono>
#include <iostream>

#include "search_server.h"

#define PROFILE_CONCAT_INTERNAL(X, Y) X ## Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
#define LOG_DURATION(x) LogDuration UNIQUE_VAR_NAME_PROFILE(x) 
#define LOG_DURATION_STREAM(x, y) LogDuration UNIQUE_VAR_NAME_PROFILE (x, y)

using namespace std::string_literals;

class LogDuration {
public:

    LogDuration(std::string text, std::ostream& stream = std::cerr) 
    :os(stream), text_(text)  {
    }


    ~LogDuration() {
        using namespace std::chrono;
        using namespace std::literals;

        const auto end_time = Clock::now();
        const auto dur = end_time - start_time_;
        os << text_ <<  ": "s << duration_cast<milliseconds>(dur).count() << " ms"s << std::endl << std::endl;
    }

private:
    using Clock = std::chrono::steady_clock;
    const Clock::time_point start_time_ = Clock::now();
    std::ostream& os = std::cerr;
    std::string text_ = "";
};

void MatchDocument(SearchServer& server, std::string raw_query, int document_id);

void FindTopDocuments(SearchServer& server, std::string raw_query);