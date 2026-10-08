#include "SearchEngine.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <sstream>
#ifdef _OPENMP
#include <omp.h>
#endif

std::vector<std::string> tokenize(const std::string &value)
{
    std::vector<std::string> out;
    std::string word;
    for (unsigned char c : value)
    {
        if (std::isalnum(c))
            word.push_back(static_cast<char>(std::tolower(c)));
        else if (!word.empty())
        {
            out.push_back(word);
            word.clear();
        }
    }
    if (!word.empty())
        out.push_back(word);
    return out;
}
static bool orderResult(const SearchResult &a, const SearchResult &b)
{
    if (std::abs(a.score - b.score) > 1e-12)
        return a.score > b.score;
    return a.id < b.id;
}
static SearchResult evaluate(const Document &d, const std::vector<std::string> &terms)
{
    SearchResult r;
    r.id = d.id;
    r.title = d.title;
    const auto words = tokenize(d.text);
    const auto lowerTitle = tokenize(d.title);
    for (const auto &term : terms)
    {
        int count = static_cast<int>(std::count(words.begin(), words.end(), term));
        count += static_cast<int>(std::count(lowerTitle.begin(), lowerTitle.end(), term));
        if (count)
        {
            r.matchedTerms.push_back(term);
            r.score += 1.0 + 0.25 * (count - 1);
        }
    }
    if (r.score > 0)
    {
        const std::size_t n = 180;
        r.snippet = d.text.substr(0, n);
        if (d.text.size() > n)
            r.snippet += "...";
    }
    return r;
}
std::vector<SearchResult> searchSequential(const std::vector<Document> &docs, const std::string &query, double *ms)
{
    auto start = std::chrono::steady_clock::now();
    auto terms = tokenize(query);
    std::vector<SearchResult> out;
    if (!terms.empty())
        for (const auto &d : docs)
        {
            auto r = evaluate(d, terms);
            if (r.score > 0)
                out.push_back(std::move(r));
        }
    std::sort(out.begin(), out.end(), orderResult);
    if (ms)
        *ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    return out;
}
std::vector<SearchResult> searchOpenMP(const std::vector<Document> &docs, const std::string &query, int threads, double *ms)
{
    auto start = std::chrono::steady_clock::now();
    auto terms = tokenize(query);
    std::vector<SearchResult> slots(docs.size());
    std::vector<unsigned char> found(docs.size(), 0);
#ifdef _OPENMP
    omp_set_num_threads(threads);
#pragma omp parallel for schedule(static)
#endif
    for (long long i = 0; i < static_cast<long long>(docs.size()); ++i)
    {
        if (!terms.empty())
        {
            auto r = evaluate(docs[static_cast<std::size_t>(i)], terms);
            if (r.score > 0)
            {
                slots[static_cast<std::size_t>(i)] = std::move(r);
                found[static_cast<std::size_t>(i)] = 1;
            }
        }
    }
    std::vector<SearchResult> out;
    for (std::size_t i = 0; i < slots.size(); ++i)
        if (found[i])
            out.push_back(std::move(slots[i]));
    std::sort(out.begin(), out.end(), orderResult);
    if (ms)
        *ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    return out;
}
bool resultsEqual(const std::vector<SearchResult> &a, const std::vector<SearchResult> &b)
{
    if (a.size() != b.size())
        return false;
    for (std::size_t i = 0; i < a.size(); ++i)
        if (a[i].id != b[i].id || a[i].matchedTerms != b[i].matchedTerms || std::abs(a[i].score - b[i].score) > 1e-9)
            return false;
    return true;
}
