#pragma once
#include <string>
#include <vector>

struct Document {
    std::string id;
    std::string title;
    std::string text;
};

struct SearchResult {
    std::string id;
    std::string title;
    double score = 0.0;
    std::vector<std::string> matchedTerms;
    std::string snippet;
};
