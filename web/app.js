const statusNode = document.getElementById('status');
const warningsNode = document.getElementById('warnings');
const timelineNode = document.getElementById('timeline');
const queryInput = document.getElementById('query-input');
const stringInput = document.getElementById('string-input');
const fileInput = document.getElementById('file-input');
const form = document.getElementById('viewer-form');

let wasmApi = null;
let uploadedText = '';

function isHttpUrl(value) {
  return /^https?:\/\//i.test(value.trim());
}

function safeHttpUrl(value) {
  try {
    const parsed = new URL(value, window.location.href);
    return parsed.protocol === 'http:' || parsed.protocol === 'https:' ? parsed.href : null;
  } catch {
    return null;
  }
}

function escapeHtml(value) {
  const div = document.createElement('div');
  div.textContent = value;
  return div.innerHTML;
}

function renderResult(result) {
  warningsNode.innerHTML = (result.warnings || [])
    .map((warning) => `<p class="warning">${escapeHtml(warning)}</p>`)
    .join('');

  if (!result.clips || result.clips.length === 0) {
    timelineNode.innerHTML = '<p>No PetaKit5D clips were found in the supplied inputs.</p>';
    return;
  }

  const maxFrames = Math.max(...result.clips.map((clip) => Math.max(clip.t || 0, 1)));
  timelineNode.innerHTML = result.clips
    .map((clip) => {
      const width = Math.max(8, Math.round(((clip.t || 1) / maxFrames) * 100));
      const duration = clip.fps > 0 ? `${(clip.t / clip.fps).toFixed(2)} s` : clip.msPerFrame > 0 ? `${((clip.msPerFrame * clip.t) / 1000).toFixed(2)} s` : 'n/a';
      const safeSource = clip.source ? safeHttpUrl(clip.source) : null;
      const source = clip.source
        ? safeSource
          ? `<p><a href="${escapeHtml(safeSource)}" target="_blank" rel="noreferrer">${escapeHtml(clip.source)}</a></p>`
          : `<p>${escapeHtml(clip.source)}</p>`
        : '';
      return `
        <article class="timeline-card">
          <strong>${escapeHtml(clip.label || 'PetaKit5D clip')}</strong>
          <div class="timeline-bar" style="width:${width}%"></div>
          <div class="meta">
            <span>x: ${clip.x || 0}</span>
            <span>y: ${clip.y || 0}</span>
            <span>z: ${clip.z || 0}</span>
            <span>c: ${clip.c || 0}</span>
            <span>t: ${clip.t || 0}</span>
            <span>duration: ${duration}</span>
          </div>
          ${clip.note ? `<p>${escapeHtml(clip.note)}</p>` : ''}
          ${source}
        </article>`;
    })
    .join('');
}

function parseWithWasm(payload) {
  if (!wasmApi) {
    throw new Error('The PetaKit5D WASM module has not loaded yet.');
  }
  const response = wasmApi.ccall('pk5d_parse', 'string', ['string'], [payload]);
  return JSON.parse(response);
}

async function loadRemoteText(value) {
  const response = await fetch(value, { method: 'GET', mode: 'cors' });
  if (!response.ok) {
    throw new Error(`Unable to fetch ${value}: ${response.status}`);
  }
  return response.text();
}

async function collectPayload() {
  const parts = [];
  const qValue = queryInput.value.trim();
  const sValue = stringInput.value.trim();

  if (qValue) {
    const safeRemoteUrl = safeHttpUrl(qValue);
    if (safeRemoteUrl) {
      parts.push(await loadRemoteText(safeRemoteUrl));
    } else {
      parts.push(qValue);
    }
  }
  if (sValue) {
    parts.push(sValue);
  }
  if (uploadedText) {
    parts.push(uploadedText);
  }
  return parts.join('\n');
}

async function renderFromInputs() {
  statusNode.textContent = 'Rendering timeline…';
  try {
    const payload = await collectPayload();
    renderResult(parseWithWasm(payload));
    statusNode.textContent = 'Timeline ready.';
  } catch (error) {
    statusNode.textContent = error.message;
  }
}

fileInput.addEventListener('change', async (event) => {
  const file = event.target.files && event.target.files[0];
  uploadedText = file ? await file.text() : '';
});

form.addEventListener('submit', async (event) => {
  event.preventDefault();
  await renderFromInputs();
});

(async () => {
  const params = new URLSearchParams(window.location.search);
  queryInput.value = params.get('q') || '';
  stringInput.value = params.get('s') || '';
  const moduleFactory = await import('./petakit5d_viewer.js');
  wasmApi = await moduleFactory.default();
  statusNode.textContent = 'WASM parser loaded.';
  if (queryInput.value || stringInput.value) {
    await renderFromInputs();
  }
})();
