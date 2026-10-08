#include "Benchmark.h"
#include "SearchEngine.h"
#include <algorithm>
#include <set>
#ifdef _OPENMP
#include <omp.h>
#endif
std::vector<BenchmarkRecord> runBenchmarks(const std::vector<Document>& docs,const std::string& query) {
    std::vector<int> counts={1,2,4,8};
#ifdef _OPENMP
    int hw=omp_get_num_procs(); if(hw>0 && std::find(counts.begin(),counts.end(),hw)==counts.end()) counts.push_back(hw);
#endif
    std::vector<BenchmarkRecord> out;
    std::vector<std::size_t> sizes;
    for(std::size_t size: {std::size_t(1000),std::size_t(5000),std::size_t(10000),std::size_t(50000),std::size_t(100000)})
        if(size<=docs.size()) sizes.push_back(size);
    if (sizes.empty() || sizes.back()!=docs.size()) sizes.push_back(docs.size());
    for(std::size_t size: sizes) {
        std::vector<Document> subset(docs.begin(),docs.begin()+static_cast<std::ptrdiff_t>(size));
        auto subSeq=searchSequential(subset,query); double st=0; searchSequential(subset,query,&st);
        for(int t:counts) { double pt=0; auto par=searchOpenMP(subset,query,t,&pt); double speed=pt>0?st/pt:0; out.push_back({size,t,st,pt,speed,speed*100.0/t,par.size(),resultsEqual(subSeq,par)}); }
    }
    return out;
}
