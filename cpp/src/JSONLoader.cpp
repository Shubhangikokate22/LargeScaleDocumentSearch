#include "JSONLoader.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

using nlohmann::json;

namespace {
bool isStringField(const json& item, const char* name) {
    return item.contains(name) && item[name].is_string();
}

std::string valueAsString(const json& value) {
    return value.is_string() ? value.get<std::string>() : value.dump();
}

void appendRecord(const json& item, std::size_t row, std::vector<Document>& docs) {
    if (!item.is_object()) return;

    // Keep the dataset's field mapping here so other Kaggle schemas are easy to adapt.
    const char* titleField = isStringField(item, "title") ? "title" : "headline";
    const char* textField = isStringField(item, "text") ? "text" : "short_description";
    if (!isStringField(item, textField) && isStringField(item, "description")) textField = "description";
    if (!isStringField(item, textField) && isStringField(item, "content")) textField = "content";
    if (!isStringField(item, titleField) && !isStringField(item, textField)) return;

    Document doc;
    doc.id = item.contains("id") ? valueAsString(item["id"]) : std::to_string(row + 1);
    doc.title = isStringField(item, titleField) ? item[titleField].get<std::string>() : "Untitled document";
    // A headline-only record remains searchable through its text field too.
    doc.text = isStringField(item, textField) ? item[textField].get<std::string>() : doc.title;
    docs.push_back(std::move(doc));
}

bool parseLineObject(const std::string& line, json& item) {
    try {
        item = json::parse(line);
        return item.is_object();
    } catch (...) {
        return false;
    }
}
}

std::vector<Document> JSONLoader::load(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Dataset file not found or cannot be opened: " + path);

    // The provided HuffPost Kaggle file is JSON Lines: one document object per line.
    // Read records individually to avoid building one enormous temporary JSON tree.
    std::string firstLine;
    while (std::getline(in, firstLine) && firstLine.find_first_not_of(" \t\r") == std::string::npos) {}
    if (firstLine.empty()) throw std::runtime_error("Dataset file is empty.");

    json firstObject;
    std::vector<Document> docs;
    const bool wrapperObject = parseLineObject(firstLine, firstObject) &&
        ((firstObject.contains("documents") && firstObject["documents"].is_array()) ||
         (firstObject.contains("data") && firstObject["data"].is_array()));
    if (parseLineObject(firstLine, firstObject) && !wrapperObject) {
        appendRecord(firstObject, 0, docs);
        std::string line;
        std::size_t row = 1;
        while (std::getline(in, line)) {
            if (line.find_first_not_of(" \t\r") == std::string::npos) continue;
            json item;
            if (parseLineObject(line, item)) appendRecord(item, row, docs);
            ++row;
        }
    } else {
        // Standard JSON array or wrapper object (e.g. {"documents": [...]}).
        in.clear();
        in.seekg(0);
        json root;
        try { in >> root; }
        catch (const std::exception& e) { throw std::runtime_error(std::string("Invalid JSON: ") + e.what()); }
        const json* rows = &root;
        if (root.is_object()) {
            if (root.contains("documents") && root["documents"].is_array()) rows = &root["documents"];
            else if (root.contains("data") && root["data"].is_array()) rows = &root["data"];
            else throw std::runtime_error("Expected a JSON array, an object containing 'documents' or 'data', or JSON Lines.");
        }
        if (!rows->is_array()) throw std::runtime_error("Dataset must be a JSON array of documents or JSON Lines.");
        for (std::size_t i = 0; i < rows->size(); ++i) appendRecord((*rows)[i], i, docs);
    }

    if (docs.empty()) throw std::runtime_error("No usable documents found. Records need 'text'/'short_description' or 'title'/'headline' fields.");
    return docs;
}
