#pragma once
#include "Document.h"
#include <string>
#include <vector>
std::vector<std::string> tokenize(const std::string& value);
std::vector<SearchResult> searchSequential(const std::vector<Document>& documents, const std::string& query, double* elapsedMs = nullptr);
std::vector<SearchResult> searchOpenMP(const std::vector<Document>& documents, const std::string& query, int threadCount, double* elapsedMs = nullptr);
bool resultsEqual(const std::vector<SearchResult>& a, const std::vector<SearchResult>& b);
