#include "JSONLoader.h"
#include "SearchEngine.h"
#include "Benchmark.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include <nlohmann/json.hpp>
using nlohmann::json;
static void help() { std::cout << "Document Search CLI\nUsage:\n  search search --mode sequential|openmp --query TEXT --dataset PATH [--threads N]\n  search correctness --query TEXT --dataset PATH [--threads N]\n  search benchmark --query TEXT --dataset PATH\n  search info --dataset PATH\n"; }
static json resultJson(const std::vector<SearchResult> &rs)
{
    json a = json::array();
    for (const auto &r : rs)
        a.push_back({{"id", r.id}, {"title", r.title}, {"score", r.score}, {"matched_terms", r.matchedTerms}, {"snippet", r.snippet}});
    return a;
}
int main(int argc, char **argv)
{
    try
    {
        if (argc < 2)
        {
            help();
            return 0;
        }
        for (int i = 1; i < argc; ++i)
            if (std::string(argv[i]) == "--help" || std::string(argv[i]) == "-h")
            {
                help();
                return 0;
            }
        std::string command = argv[1], mode = "sequential", query, dataset;
        int threads = 4;
        for (int i = 2; i < argc; ++i)
        {
            std::string k = argv[i];
            if ((k == "--mode" || k == "--query" || k == "--dataset" || k == "--threads") && i + 1 < argc)
            {
                std::string v = argv[++i];
                if (k == "--mode")
                    mode = v;
                else if (k == "--query")
                    query = v;
                else if (k == "--dataset")
                    dataset = v;
                else
                    threads = std::stoi(v);
            }
            else
                throw std::runtime_error("Unknown or incomplete argument: " + k);
        }
        if (dataset.empty())
            throw std::runtime_error("Please provide --dataset PATH.");
        if (threads < 1 || threads > 1024)
            throw std::runtime_error("Thread count must be between 1 and 1024.");
        auto docs = JSONLoader::load(dataset);
        if (command == "info")
        {
            std::size_t words = 0;
            json preview = json::array();
            for (std::size_t i = 0; i < docs.size(); ++i)
            {
                auto n = tokenize(docs[i].text).size();
                words += n;
                if (i < 5)
                    preview.push_back({{"id", docs[i].id}, {"title", docs[i].title}, {"text", docs[i].text.substr(0, 200)}});
            }
            std::cout << json({{"dataset_size", docs.size()}, {"total_words", words}, {"average_document_length", docs.empty() ? 0.0 : double(words) / docs.size()}, {"preview", preview}}).dump();
            return 0;
        }
        if (command == "search")
        {
            if (query.empty())
                throw std::runtime_error("Query must not be empty.");
            double ms = 0;
            std::vector<SearchResult> rs;
            if (mode == "sequential")
                rs = searchSequential(docs, query, &ms);
            else if (mode == "openmp")
                rs = searchOpenMP(docs, query, threads, &ms);
            else
                throw std::runtime_error("Mode must be sequential or openmp.");
            std::cout << json({{"mode", mode}, {"threads", mode == "openmp" ? threads : 1}, {"dataset_size", docs.size()}, {"execution_time_ms", ms}, {"result_count", rs.size()}, {"results", resultJson(rs)}}).dump();
            return 0;
        }
        if (command == "correctness")
        {
            if (query.empty())
                throw std::runtime_error("Query must not be empty.");
            auto seq = searchSequential(docs, query);
            auto par = searchOpenMP(docs, query, threads);
            std::size_t matched = 0;
            for (std::size_t i = 0; i < std::min(seq.size(), par.size()); ++i)
                if (seq[i].id == par[i].id)
                    ++matched;
            std::cout << json({{"correct", resultsEqual(seq, par)}, {"sequential_results", seq.size()}, {"parallel_results", par.size()}, {"matching_results", matched}, {"sequential", resultJson(seq)}, {"parallel", resultJson(par)}}).dump();
            return 0;
        }
        if (command == "benchmark")
        {
            if (query.empty())
                throw std::runtime_error("Query must not be empty.");
            auto records = runBenchmarks(docs, query);
            json a = json::array();
            for (auto r : records)
                a.push_back({{"dataset_size", r.datasetSize}, {"query", query}, {"threads", r.threads}, {"sequential_time_ms", r.sequentialTimeMs}, {"parallel_time_ms", r.parallelTimeMs}, {"speedup", r.speedup}, {"efficiency", r.efficiency}, {"result_count", r.resultCount}, {"correctness", r.correctness}});
            std::cout << json({{"experiments", a}}).dump();
            return 0;
        }
        throw std::runtime_error("Unknown command: " + command);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << "\n";
        return 2;
    }
}
