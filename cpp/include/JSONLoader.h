#pragma once
#include "Document.h"
#include <string>
#include <vector>

class JSONLoader {
public:
    static std::vector<Document> load(const std::string& path);
};
