#include "remove_duplicates.h"

// скажу честно, я вам не поверил, что так будет эффективней, ведь по ощущениям вставлять элементы в 
// set<set<string>> дорогое удовольствие, а в случае с сортировокй я ничего не переписываю, а только   
// сравниваю mapы и сортирую vector<int> за O(NlogN). Но провел тесты на 1000 документов-дубликатов  
// и на 1000 уникальных документов,этот способ отрабатывал за 1-2ms, мой способ с сортировкой за 700+ms...
void RemoveDuplicates(SearchServer& server) {
    std::set<std::set<std::string>> unique_docs_words;
    std::vector<int> delete_list;
    for (const int id : server) {
        std::set<std::string> docs_words;
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