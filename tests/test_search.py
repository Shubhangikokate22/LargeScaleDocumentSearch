"""Small integration tests; build the C++ executable before running."""
import json, os, subprocess, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
EXE=Path(os.environ.get('SEARCH_EXECUTABLE',ROOT/'build'/'cpp'/'search.exe'))
if not EXE.exists(): EXE=ROOT/'build'/'cpp'/'search'
DATA=ROOT/'tests'/'test_dataset.json'
def run(*args):
    p=subprocess.run([str(EXE),*map(str,args)],cwd=ROOT,capture_output=True,text=True)
    return p.returncode,json.loads(p.stdout) if p.returncode==0 else p.stderr
class SearchTests(unittest.TestCase):
    def test_dataset_info(self): self.assertEqual(run('info','--dataset',DATA)[1]['dataset_size'],3)
    def test_sequential_search(self): self.assertEqual(run('search','--mode','sequential','--query','machine learning','--dataset',DATA)[1]['result_count'],2)
    def test_openmp_search(self): self.assertEqual(run('search','--mode','openmp','--query','machine learning','--threads','2','--dataset',DATA)[1]['result_count'],2)
    def test_correctness(self): self.assertTrue(run('correctness','--query','machine learning','--dataset',DATA)[1]['correct'])
    def test_empty_query(self): self.assertNotEqual(run('search','--mode','sequential','--query','','--dataset',DATA)[0],0)
    def test_no_results(self): self.assertEqual(run('search','--mode','sequential','--query','xylophone','--dataset',DATA)[1]['result_count'],0)
    def test_invalid_dataset(self): self.assertNotEqual(run('info','--dataset',ROOT/'missing.json')[0],0)
if __name__=='__main__': unittest.main()
