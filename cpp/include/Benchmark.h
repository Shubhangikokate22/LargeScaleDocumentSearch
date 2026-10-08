#pragma once
#include "Document.h"
#include <string>
#include <vector>
struct BenchmarkRecord {
    std::size_t datasetSize; int threads; double sequentialTimeMs; double parallelTimeMs;
    double speedup; double efficiency; std::size_t resultCount; bool correctness;
};
std::vector<BenchmarkRecord> runBenchmarks(const std::vector<Document>& documents, const std::string& query);
