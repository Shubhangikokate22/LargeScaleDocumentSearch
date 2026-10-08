from pathlib import Path
import json
import os
import subprocess
from flask import Flask, jsonify, request, send_from_directory
from flask_cors import CORS

ROOT = Path(__file__).resolve().parents[1]
APP = Flask(__name__, static_folder=str(ROOT / "frontend"), static_url_path="")
CORS(APP)
DEFAULT_DATASET = ROOT / "data" / "dataset.json"
RESULTS = ROOT / "results"
RESULTS.mkdir(exist_ok=True)
BENCHMARK_FILE = RESULTS / "benchmark_results.json"
EXECUTABLE = Path(os.environ.get("SEARCH_EXECUTABLE", ROOT / "build" / "cpp" / "search.exe"))
if not EXECUTABLE.exists():
    for candidate in (ROOT / "build" / "cpp" / "Release" / "search.exe",
                      ROOT / "build" / "cpp" / "Debug" / "search.exe",
                      ROOT / "build" / "cpp" / "search"):
        if candidate.exists():
            EXECUTABLE = candidate
            break

def invoke(args):
    if not EXECUTABLE.exists():
        raise RuntimeError("C++ executable is missing. Build the project using the README instructions.")
    p = subprocess.run([str(EXECUTABLE), *map(str, args)], cwd=ROOT, capture_output=True,
                       text=True, encoding="utf-8", errors="replace", timeout=300)
    if p.returncode:
        raise ValueError(p.stderr.strip() or "C++ operation failed.")
    try: return json.loads(p.stdout)
    except json.JSONDecodeError as e: raise RuntimeError("C++ program returned malformed JSON.") from e

def dataset_arg(data):
    path = Path(data.get("dataset", str(DEFAULT_DATASET))).expanduser()
    if not path.is_absolute(): path = (ROOT / path).resolve()
    return path

@APP.get("/")
def index(): return send_from_directory(APP.static_folder, "index.html")

@APP.post("/api/search")
def search():
    data=request.get_json(silent=True) or {}; query=str(data.get("query", "")).strip()
    if not query: return jsonify(error="Enter a search query."),400
    mode=data.get("mode","sequential")
    if mode not in ("sequential","openmp"): return jsonify(error="Mode must be sequential or openmp."),400
    try:
        threads=int(data.get("threads",4))
        if threads<1 or threads>1024: raise ValueError("Thread count must be between 1 and 1024.")
        dataset=dataset_arg(data)
        if not dataset.is_file(): raise ValueError(f"Dataset file not found: {dataset}")
        out=invoke(["search","--mode",mode,"--query",query,"--dataset",dataset,"--threads",threads])
        (RESULTS/"search_results.json").write_text(json.dumps(out,indent=2),encoding="utf-8")
        return jsonify(out)
    except (ValueError,RuntimeError,subprocess.TimeoutExpired) as e: return jsonify(error=str(e)),400

@APP.post("/api/benchmark")
def benchmark():
    data=request.get_json(silent=True) or {}; query=str(data.get("query", "machine learning")).strip()
    if not query:return jsonify(error="Enter a benchmark query."),400
    try:
        out=invoke(["benchmark","--query",query,"--dataset",dataset_arg(data)])
        out["dataset_filename"] = dataset_arg(data).name
        out["dataset_size"] = max((int(row.get("dataset_size", 0)) for row in out.get("experiments", [])), default=0)
        BENCHMARK_FILE.write_text(json.dumps(out,indent=2),encoding="utf-8")
        return jsonify(out)
    except (ValueError,RuntimeError,subprocess.TimeoutExpired) as e:return jsonify(error=str(e)),400

@APP.post("/api/correctness")
def correctness():
    data=request.get_json(silent=True) or {}; query=str(data.get("query", "machine learning")).strip()
    try:
        out=invoke(["correctness","--query",query,"--dataset",dataset_arg(data),"--threads",int(data.get("threads",4))])
        (RESULTS/"correctness_results.json").write_text(json.dumps(out,indent=2),encoding="utf-8")
        return jsonify(out)
    except (ValueError,RuntimeError,subprocess.TimeoutExpired) as e:return jsonify(error=str(e)),400

@APP.get("/api/results")
def results():
    data=request.args; path=dataset_arg(data)
    try:
        info=invoke(["info","--dataset",path])
        benchmarks=json.loads(BENCHMARK_FILE.read_text(encoding="utf-8")) if BENCHMARK_FILE.exists() else {"experiments":[]}
        measured_size=max((int(row.get("dataset_size",0)) for row in benchmarks.get("experiments",[])),default=0)
        benchmark_dataset={"filename":benchmarks.get("dataset_filename",path.name),"size":benchmarks.get("dataset_size",measured_size)}
        return jsonify({"dataset_filename":path.name,"dataset_path":str(path),"dataset_size_bytes":path.stat().st_size,"loaded":True,**info,"benchmarks":benchmarks.get("experiments",[]),"benchmark_dataset":benchmark_dataset})
    except (ValueError,RuntimeError,subprocess.TimeoutExpired) as e:return jsonify(error=str(e),loaded=False),400

@APP.errorhandler(Exception)
def unexpected_error(_error):
    APP.logger.exception("Unhandled application error")
    return jsonify(error="The server could not complete that request. Check the dataset path and try again."),500

if __name__ == "__main__": APP.run(host="127.0.0.1",port=5000,debug=False)
