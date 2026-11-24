// Small helpers shared by every module page.
export async function api(path, { method = 'GET', body, form } = {}) {
  const opts = { method, headers: {} };
  if (form) opts.body = form;
  else if (body !== undefined) { opts.body = JSON.stringify(body); opts.headers['Content-Type'] = 'application/json'; }
  const r = await fetch(path, opts);
  let data = null;
  try { data = await r.json(); } catch { /* empty */ }
  if (!r.ok) {
    const msg = (data && (data.detail?.[0]?.msg || data.detail)) || `Fehler ${r.status}`;
    throw new Error(typeof msg === 'string' ? msg : JSON.stringify(msg));
  }
  return data;
}

export function toast(text, kind = 'info', ms = 3500) {
  const el = document.createElement('div');
  el.className = `toast ${kind}`;
  el.textContent = text;
  document.getElementById('toasts').append(el);
  setTimeout(() => el.remove(), ms);
}

export function esc(s) {
  return String(s ?? '').replace(/[&<>"']/g, c => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));
}

// html`...` builds a DocumentFragment-free string; use with el.innerHTML. Values are escaped unless wrapped in raw().
export const raw = s => ({ __raw: String(s) });
export function html(strings, ...vals) {
  return strings.reduce((out, s, i) => {
    let v = i < vals.length ? vals[i] : '';
    if (Array.isArray(v)) v = v.map(x => (x && x.__raw !== undefined ? x.__raw : esc(x))).join('');
    else if (v && v.__raw !== undefined) v = v.__raw;
    else v = esc(v);
    return out + s + v;
  }, '');
}

export async function copy(text, label = 'Kopiert') {
  try { await navigator.clipboard.writeText(text); }
  catch {
    const ta = document.createElement('textarea'); ta.value = text; document.body.append(ta); ta.select();
    document.execCommand('copy'); ta.remove();
  }
  toast(`${label} ✔`, 'ok', 1500);
}

export const fmt = n => (n == null ? '–' : n >= 1e6 ? (n / 1e6).toFixed(1) + ' Mio' : n >= 1e3 ? (n / 1e3).toFixed(1) + 'k' : String(n));

export function store(key, value) {
  try {
    if (value === undefined) return JSON.parse(localStorage.getItem('bd.' + key));
    localStorage.setItem('bd.' + key, JSON.stringify(value));
  } catch { return null; }
}
