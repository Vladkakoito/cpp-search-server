#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& server) {
    std::set<std::set<std::string_view>> unique_docs_words;
    std::vector<int> delete_list;
    for (const int id : server) {
        std::set<std::string_view> docs_words;
        for (const auto& [word, _] : server.GetWordFrequencies(id)) {
            docs_words.insert(word);
        }
        if (unique_docs_words.count(docs_words)) {
            delete_list.push_back(id);
        } else {
            unique_docs_words.insert(docs_words);
        }
    }
    for (const int id : delete_list) {
        std::cout << "Found duplicate document id "s << id << std::endl;
        server.RemoveDocument(id);
    }
}