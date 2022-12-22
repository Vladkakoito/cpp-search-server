#pragma once

#include "search_server.h"

bool IsEqualDocuments(int id_1, int id_2, const SearchServer& server);

bool HasGreaterDocument (int lhs, int rhs, const SearchServer& server);

void RemoveDuplicates(SearchServer& server);