"""Run one real CLI benchmark and store the returned measurements."""
import argparse, json, subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--dataset',default='data/dataset.json');p.add_argument('--query',default='machine learning');p.add_argument('--executable',default='build/cpp/search');a=p.parse_args()
exe=Path(a.executable);exe=exe if exe.is_absolute() else root/exe
out=subprocess.run([str(exe),'benchmark','--dataset',str(Path(a.dataset).resolve()),'--query',a.query],cwd=root,capture_output=True,text=True,check=True)
data=json.loads(out.stdout); (root/'results'/'benchmark_results.json').write_text(json.dumps(data,indent=2),encoding='utf-8'); print(f"Saved {len(data['experiments'])} measured experiments.")
