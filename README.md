# LARGE-SCALE DOCUMENT SEARCH USING SEQUENTIAL AND OPENMP PARALLEL PROCESSING

A college-level parallel computing project that searches a user-provided JSON collection. The search algorithm is written in C++17; Flask only provides a small API and serves the vanilla HTML/CSS/JavaScript dashboard.

## Problem and objectives

Searching documents one at a time is simple but takes longer as collections grow. This project compares a sequential scan with a parallel scan using OpenMP. It reports matching documents and relevance scores, checks that both versions agree, and measures execution time, speedup, and parallel efficiency.

## Technology and layout

- C++17, CMake, OpenMP, and nlohmann/json for the core.
- Python 3, Flask, and flask-cors for the API.
- HTML, CSS, vanilla JavaScript, and Chart.js for the dashboard.

```text
large-scale-document-search/
├── CMakeLists.txt
├── cpp/ (include/, src/, CMakeLists.txt)
├── backend/ (app.py, requirements.txt)
├── frontend/ (index.html, css/, js/)
├── data/dataset.json
├── results/ (benchmark_results.json, search_results.json, correctness_results.json)
├── scripts/ (run_benchmark.py, generate_graphs.py)
├── tests/ (test_dataset.json, test_search.py)
├── README.md
└── run.bat
```

`cpp/include/Document.h` defines the document and result records. `JSONLoader` reads arrays and objects with `documents` or `data` arrays. `SearchEngine.cpp` tokenizes lowercased text and query terms, gives one point per matched query term plus 0.25 per repeated occurrence, and sorts by score then ID. The sequential implementation visits each document in order. The OpenMP implementation parallelizes that same document loop; each iteration writes to its own vector slot, avoiding shared result writes. The results are compacted and sorted after the parallel region.

OpenMP is used because it expresses loop parallelism directly in C++ with a small amount of code and works with shared-memory CPUs. Each worker is a thread assigned some document iterations. `Benchmark.cpp` measures searches with `std::chrono`; the benchmark includes both thread scaling and the available dataset-size steps (1k, 5k, 10k, 50k, 100k), skipping sizes the input does not contain. If the dataset is smaller than 1k, the full available collection is still measured.

## Dataset format

Replace `data/dataset.json` with your Kaggle JSON file. Do not add the dataset to source control if it is large or licensed for restricted use. The default fields are `id`, `title`, and `text`; records lacking a string `text` are skipped. Missing IDs receive row numbers and missing titles use `Untitled document`. Top-level arrays and `{ "documents": [...] }` / `{ "data": [...] }` are supported. To adapt field names, edit the mapping in `cpp/src/JSONLoader.cpp`.

```json
[
  {"id": 1, "title": "Example Document", "text": "Document content goes here."}
]
```

The placeholder `data/dataset.json` is empty. The included `tests/test_dataset.json` is only a small test fixture, not the project dataset.

## Windows setup and build (PowerShell)

Install CMake, Python 3, and MSYS2 MinGW-w64 g++ with OpenMP support. Install the nlohmann JSON headers in the compiler's include path (for MSYS2 UCRT64, `pacman -S mingw-w64-ucrt-x86_64-nlohmann-json mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake`). Make sure CMake uses the matching MSYS2 compiler, for example:

```powershell
cmake -S . -B build -G "MinGW Makefiles" `
  -DCMAKE_CXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe
cmake --build build --config Release
```

The CMake configuration detects OpenMP. If unavailable, CMake still builds the sequential program, but the parallel loop is executed by one thread; install an OpenMP-capable compiler for the assignment's parallel experiments. Run `.un.bat` to build, create a virtual environment, install the two Flask packages, and start the server at http://127.0.0.1:5000. For manual Python setup:

```powershell
py -3 -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install -r backend\requirements.txt
python backend\app.py
```

## Running the CLI

Run from the project root. With MSYS2 MinGW Makefiles the executable is usually `build\cpp\search.exe`.

```powershell
build\cpp\search.exe --help
build\cpp\search.exe search --mode sequential --query "machine learning" --dataset data/dataset.json
build\cpp\search.exe search --mode openmp --query "machine learning" --dataset data/dataset.json --threads 4
build\cpp\search.exe correctness --query "machine learning" --dataset data/dataset.json --threads 4
build\cpp\search.exe benchmark --query "machine learning" --dataset data/dataset.json
```

The web UI has Search, Performance, Dataset, and Correctness sections. Set the dataset path in Search (the other sections use it too), then run searches or experiments. The API uses Python `subprocess` argument arrays, not a shell command string. It does not implement search in Python.

## Benchmarks, charts, and correctness

Benchmark values are obtained from actual C++ runs and written to `results/benchmark_results.json`; the starter file contains no measurements. The UI displays “No benchmark data available” until a benchmark has been run. `python scripts/run_benchmark.py --dataset data/dataset.json --query "machine learning"` runs one and saves records. The dashboard charts consume those JSON records. `python scripts/generate_graphs.py` can generate a standalone measured-data chart at `results/benchmark_graphs.html`.

For each thread count, the benchmark runs sequential search and OpenMP search against the same subset. Speedup is `sequential_time_ms / parallel_time_ms`; efficiency is `speedup / threads * 100`. If elapsed time is zero, speedup is reported as zero to avoid division by zero. Correctness compares result count, ordered document IDs, matched terms, and scores within 1e-9. Ties sort by ID so output order is deterministic. `python -m unittest tests/test_search.py` runs the small CLI integration tests after building.

## Bottlenecks, limitations, and expected observations

The interface derives its summary from measured timing records. Parallel overhead can dominate small inputs; at higher thread counts, speedup can plateau due to available CPU cores, memory traffic, and coordination. The dashboard does not label these as measured causes; it reports timing behavior and calls out the possible overhead when smaller measured parallel runs are slower. Larger datasets commonly expose more useful parallel work, but actual outcomes depend on hardware and text lengths.

This is a simple term-frequency ranker, not a production search engine: it has no stemming, stop-word removal, inverted index, persistence database, or sophisticated relevance model. Text is decoded as UTF-8 by JSON, but tokenization is intentionally ASCII alphanumeric for clarity. Timing includes tokenization and result sorting, while JSON load time is outside the timed search. Results are illustrative and machine-dependent. For a viva, explain the shared document vector, independent indexed writes, deterministic final sort, equality check, and the speedup/efficiency formulas.

## Tests

After building the C++ program, run `python tests/test_search.py`. The tests cover JSON loading, sequential and OpenMP searches, correctness, empty query, no matches, and invalid paths.
