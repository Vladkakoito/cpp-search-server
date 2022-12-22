#include "remove_duplicates.h"

bool IsEqualDocuments(int lhs_id, int rhs_id, SearchServer& server) {
    const std::map<std::string, double>& lhs = server.GetWordFrequencies(lhs_id);
    const std::map<std::string, double>& rhs = server.GetWordFrequencies(rhs_id);
    auto lhs_it = lhs.begin();
    auto rhs_it = rhs.begin();
    if (std::distance(lhs_it, lhs.end()) != std::distance(rhs_it, rhs.end())) {
        return false;
    }
    while (lhs_it != lhs.end()) {
        if ((*lhs_it).first != (*rhs_it).first) {
            return false;
        }
        std::advance(lhs_it, 1);
        std::advance(rhs_it, 1);
    }
    return true;
}

bool HasGreaterDocument (int lhs_id, int rhs_id, const SearchServer& server) {
    const std::map<std::string, double>& lhs = server.GetWordFrequencies(lhs_id);
    const std::map<std::string, double>& rhs = server.GetWordFrequencies(rhs_id);
    if (lhs.empty()) {
        return true;
    }
    if (rhs.empty()) {
        return false;
    }
    auto lhs_it = lhs.begin();
    auto rhs_it = rhs.begin();
    while ((*lhs_it).first == (*rhs_it).first) {
        std::advance(lhs_it, 1);
        std::advance(rhs_it, 1);
        if (lhs_it == lhs.end()) {
            if (rhs_it == rhs.end()) {
                return lhs_id < rhs_id;
            }
            return true;
        }
        if (rhs_it == rhs.end()) {
            return false;
        }
    }
    return (*lhs_it).first < (*rhs_it).first;
}   

// В этой функции я сортирую копию списка ID всех документов лексикографически, по составу их слов.
// После чего, сравниваю соседние элементы, и если одинаковые - добавляю в список на удаление.
void RemoveDuplicates(SearchServer& server) {

    std::vector<int> sorted_ids(server.begin(), server.end()); 

    std::sort(sorted_ids.begin(), sorted_ids.end(), [server](int lhs_id, int rhs_id) {
                                            return HasGreaterDocument(lhs_id, rhs_id, server);});

    std::set<int> delete_list;
    for (auto it = sorted_ids.begin(); it != sorted_ids.end(); std::advance(it, 1)) {
        for (auto next_it = std::next(it); next_it != sorted_ids.end(); std::advance(next_it, 1)) {
            if (IsEqualDocuments(*it, *next_it, server)) {
                delete_list.insert(*next_it);
            } else {
                it = next(next_it, -1);
                break;
            }
        }
    }
    for (int id : delete_list) {
        std::cout << "Found duplicate document id "s << id << std::endl;
        server.RemoveDocument(id);
    }
}