const $ = s => document.querySelector(s);
const $$ = s => [...document.querySelectorAll(s)];
function esc(value) {
  return String(value ?? '').replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
}
const payload = () => ({dataset: $('#datasetPath').value.trim() || 'data/dataset.json'});
async function api(url, body) {
  const response = await fetch(url, body ? {method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)} : {});
  let data;
  try { data = await response.json(); } catch { throw Error('The server returned an unreadable response.'); }
  if (!response.ok) throw Error(data.error || 'Request failed.');
  return data;
}

function updateThreadControl() {
  const sequential = $('#mode').value === 'sequential';
  $('#threads').disabled = sequential;
  $('#threadsLabel').classList.toggle('disabled-control', sequential);
}
$('#mode').addEventListener('change', updateThreadControl);
updateThreadControl();

$$('.tabs button').forEach(button => button.onclick = () => {
  $$('.tabs button').forEach(x => x.classList.remove('active'));
  $$('.page').forEach(x => x.classList.remove('active'));
  button.classList.add('active');
  $('#'+button.dataset.page).classList.add('active');
  if (button.dataset.page === 'dataset') loadDataset();
  if (button.dataset.page === 'performance') loadBenchmarks();
});

$('#searchBtn').onclick = async () => {
  const error = $('#searchError');
  error.textContent = '';
  $('#searchResults').innerHTML = '<div class="empty">Searching…</div>';
  try {
    const data = await api('/api/search', {...payload(),query:$('#query').value,mode:$('#mode').value,threads:+$('#threads').value});
    $('#totalResults').textContent = Number(data.result_count).toLocaleString();
    $('#searchTime').textContent = Number(data.execution_time_ms).toFixed(3)+' ms';
    $('#searchMode').textContent = data.mode === 'openmp' ? `OpenMP · ${data.threads} threads` : 'Sequential · 1 thread';
    const top = data.results.slice(0,10);
    $('#resultSummary').textContent = data.result_count
      ? `Showing the top ${top.length} of ${Number(data.result_count).toLocaleString()} matching documents, ranked by relevance.`
      : 'No documents matched this query.';
    $('#searchResults').className = 'results';
    $('#searchResults').innerHTML = top.length ? top.map(result => `<article class="result"><div class="result-head"><div><h4>${esc(result.title)}</h4><span class="docid">Document ID · ${esc(result.id)}</span></div><span class="score">Score ${Number(result.score).toFixed(2)}</span></div><p class="snippet">${esc(result.snippet)}</p><div class="chips">${result.matched_terms.map(term=>`<span class="chip">${esc(term)}</span>`).join('')}</div></article>`).join('') : '<div class="panel empty">No documents matched this query.</div>';
  } catch (e) {
    error.textContent = e.message;
    $('#searchResults').innerHTML = '<div class="empty">Search could not be completed.</div>';
  }
};

function showBenchmarkDataset(filename, size, rowCount) {
  const current = filename || payload().dataset.split(/[\\/]/).pop();
  const existing = rowCount ? `Benchmark records use ${esc(filename || current)} (${Number(size||0).toLocaleString()} documents).` : `Ready to benchmark ${esc(current)} using its full document collection.`;
  $('#benchDatasetInfo').textContent = existing;
}
function renderTable(rows) {
  $('#benchRows').innerHTML = rows.length ? rows.map(row=>`<tr><td>${Number(row.dataset_size).toLocaleString()}</td><td>${row.threads}</td><td>${Number(row.sequential_time_ms).toFixed(3)} ms</td><td>${Number(row.parallel_time_ms).toFixed(3)} ms</td><td>${Number(row.speedup).toFixed(2)}×</td><td>${Number(row.efficiency).toFixed(1)}%</td><td>${row.correctness?'✓':'✗'}</td></tr>`).join('') : '<tr><td colspan="7" class="empty">No benchmark data available.</td></tr>';
  if (!rows.length) return;
  renderCharts(rows);
  const best = rows.reduce((a,b)=>b.speedup>a.speedup?b:a);
  $('#seqTime').textContent = best.sequential_time_ms.toFixed(3)+' ms';
  $('#parTime').textContent = best.parallel_time_ms.toFixed(3)+' ms';
  $('#speedup').textContent = best.speedup.toFixed(2)+'×';
  $('#efficiency').textContent = best.efficiency.toFixed(1)+'%';
  const minSize=Math.min(...rows.map(x=>x.dataset_size)), maxSize=Math.max(...rows.map(x=>x.dataset_size));
  const small=rows.filter(x=>x.dataset_size===minSize), large=rows.filter(x=>x.dataset_size===maxSize);
  let note=`Measured speedup ranged from ${Math.min(...rows.map(x=>x.speedup)).toFixed(2)}× to ${best.speedup.toFixed(2)}× across recorded runs.`;
  if(small[0].parallel_time_ms>small[0].sequential_time_ms) note+=' At the smallest measured size, parallel execution was slower, consistent with thread startup and coordination overhead.';
  if(large[0].speedup<best.speedup) note+=' The largest workload did not reach the best measured speedup; memory access costs and per-thread work may affect the curve.';
  $('#analysis').textContent=note;
}
async function loadBenchmarks() {
  try {
    const data=await api('/api/results?'+new URLSearchParams(payload()));
    const rows=data.benchmarks||[];
    if(rows.length) showBenchmarkDataset(data.benchmark_dataset?.filename,data.benchmark_dataset?.size,rows.length);
    else $('#benchDatasetInfo').textContent=`Ready to benchmark ${data.dataset_filename} (${Number(data.dataset_size).toLocaleString()} documents).`;
    renderTable(rows);
  } catch(e) {
    $('#benchDatasetInfo').textContent=e.message;
    $('#benchRows').innerHTML=`<tr><td colspan="7" class="empty">${esc(e.message)}</td></tr>`;
  }
}
$('#benchBtn').onclick=async()=>{
  const status=$('#benchError');status.textContent='Running measured benchmarks…';
  try {
    const data=await api('/api/benchmark',{...payload(),query:$('#benchQuery').value});
    renderTable(data.experiments||[]);
    showBenchmarkDataset(data.dataset_filename,data.dataset_size,(data.experiments||[]).length);
    status.textContent='Benchmark complete. Actual measurements were saved.';
  } catch(e) { status.textContent=e.message; }
};

async function loadDataset() {
  try {
    const data=await api('/api/results?'+new URLSearchParams(payload()));
    $('#dsFilename').textContent=data.dataset_filename;
    $('#dsCount').textContent=Number(data.dataset_size).toLocaleString();
    $('#dsWords').textContent=Number(data.total_words).toLocaleString();
    $('#dsAverage').textContent=Number(data.average_document_length).toFixed(1);
    $('#dsSize').textContent='Dataset size · '+(Number(data.dataset_size_bytes)/1048576).toFixed(2)+' MB';
    $('#loadedStatus').textContent='Loaded successfully';
    $('#preview').className='results';
    $('#preview').innerHTML=data.preview.map(doc=>`<article class="result"><h4>${esc(doc.title)}</h4><span class="docid">ID · ${esc(doc.id)}</span><p class="snippet">${esc(doc.text)}</p></article>`).join('');
  } catch(e) { $('#loadedStatus').textContent='Load failed';$('#preview').className='results empty';$('#preview').textContent=e.message; }
}
$('#refreshDataset').onclick=loadDataset;

$('#correctBtn').onclick=async()=>{
  const error=$('#correctError');error.textContent='Comparing implementations…';
  try {
    const data=await api('/api/correctness',{...payload(),query:$('#correctQuery').value,threads:+$('#threads').value});
    error.textContent='';
    const status=$('#correctStatus');status.className='verify '+(data.correct?'good':'bad');
    status.textContent=data.correct?'✓ PARALLEL IMPLEMENTATION VERIFIED':'✗ CORRECTNESS MISMATCH';
    $('#seqCount').textContent=data.sequential_results;$('#ompCount').textContent=data.parallel_results;$('#matchCount').textContent=data.matching_results;
    const count=Math.min(10,Math.max(data.sequential.length,data.parallel.length));
    $('#correctRows').innerHTML=count?Array.from({length:count},(_,i)=>{const a=data.sequential[i],b=data.parallel[i],same=a&&b&&a.id===b.id&&Math.abs(a.score-b.score)<1e-9;return `<tr><td>${i+1}</td><td>${esc(a?.id??b?.id)}</td><td>${esc(a?.title??b?.title)}</td><td>${a?Number(a.score).toFixed(2):'—'}</td><td>${b?Number(b.score).toFixed(2):'—'}</td><td>${same?'✓':'✗'}</td></tr>`}).join(''):'<tr><td colspan="6" class="empty">Both implementations returned no matches.</td></tr>';
  } catch(e) { error.textContent=e.message;$('#correctStatus').className='verify bad';$('#correctStatus').textContent='Correctness check could not run.'; }
};
loadBenchmarks();
