"""Build a standalone HTML chart from measured benchmark_results.json records."""
import json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
data=json.loads((root/'results'/'benchmark_results.json').read_text(encoding='utf-8'))
rows=data.get('experiments',[])
if not rows:
    print('No benchmark data available; run a benchmark first.')
else:
    payload=json.dumps(rows)
    html='''<!doctype html><meta charset="utf-8"><title>Document Search Benchmark Graphs</title><h1>Measured benchmark results</h1><canvas id="c"></canvas><script src="https://cdn.jsdelivr.net/npm/chart.js"></script><script>const rows=DATA;const threads=[...new Set(rows.map(r=>r.threads))].sort((a,b)=>a-b);new Chart(document.getElementById("c"),{type:"line",data:{labels:threads,datasets:[{label:"Sequential ms",data:threads.map(t=>rows.find(r=>r.threads===t).sequential_time_ms)},{label:"OpenMP ms",data:threads.map(t=>rows.find(r=>r.threads===t).parallel_time_ms)}]},options:{scales:{x:{title:{display:true,text:"Threads"}},y:{title:{display:true,text:"Execution time (ms)"},beginAtZero:true}}}})</script>'''.replace('DATA',payload)
    (root/'results'/'benchmark_graphs.html').write_text(html,encoding='utf-8'); print('Wrote results/benchmark_graphs.html using measured data.')
