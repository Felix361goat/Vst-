// Drehs & Projekte: plan shoots, tick shots/gear, track deliverables.
import { api, html, raw, toast, store } from '/static/api.js';

let META = null;
const STATUS = { geplant: ['Geplant', 'blue'], gedreht: ['Gedreht', 'purple'], schnitt: ['Im Schnitt', 'orange'], geliefert: ['Geliefert', 'green'], abgesagt: ['Abgesagt', 'red'] };
const badge = s => { const [l, c] = STATUS[s] || [s, 'gray']; return html`<span class="badge ${c}">${l}</span>`; };
const days = d => d ? Math.round((new Date(d) - new Date(new Date().toISOString().slice(0, 10))) / 864e5) : null;
const rel = d => { const n = days(d); return n == null ? '–' : n === 0 ? 'heute' : n === 1 ? 'morgen' : n < 0 ? `vor ${-n} T.` : `in ${n} T.`; };

export default {
  async mount(el) {
    META = await api('/api/shoots/meta');
    const pending = store('shoots.newFor');
    if (pending) {
      store('shoots.newFor', null);
      const s = await api('/api/shoots/shoots', { method: 'POST', body: { lead_id: pending } });
      await render(el); openShoot(s.id, () => render(el));
      return;
    }
    render(el);
  },
};

async function render(el) {
  const list = await api('/api/shoots/shoots');
  const active = list.filter(s => !['geliefert', 'abgesagt'].includes(s.status));
  const done = list.filter(s => ['geliefert', 'abgesagt'].includes(s.status));
  const card = s => {
    const shots = s.shots.length ? Math.round(100 * s.shots.filter(x => x.done).length / s.shots.length) : 0;
    const openD = s.deliverables.filter(d => !d.done).length;
    return html`<div class="card tile" style="margin:0;cursor:pointer" data-open="${s.id}">
      <div class="row"><b>${s.title}</b><span class="spacer"></span>${raw(badge(s.status))}</div>
      <div class="small muted">${META.types[s.type] || s.type} · ${s.date || 'kein Datum'} (${rel(s.date)})${s.call_time ? ' · ' + s.call_time : ''}</div>
      <div class="small muted">${s.location || ''}</div>
      <div class="progress-line"><div style="width:${shots}%"></div></div>
      <div class="small muted">Shots ${shots} % · ${openD} offene Lieferungen${s.next_deadline && s.next_deadline !== '9999' ? ' · nächste Deadline ' + rel(s.next_deadline) : ''}</div></div>`;
  };
  el.innerHTML = html`
    <div class="row" style="align-items:flex-end"><div><h1>Drehs & Projekte</h1><p class="lead">Shotlist, Equipment, Wetter & Golden Hour, Call Sheet und Deadlines.</p></div>
      <span class="spacer"></span><select id="type" style="width:auto">${Object.entries(META.types).map(([k, l]) => raw(html`<option value="${k}">${l}</option>`))}</select>
      <button class="btn primary" id="new">＋ Neuer Dreh</button></div>
    ${raw(active.length ? html`<div class="tiles">${active.map(s => raw(card(s)))}</div>`
      : '<div class="card empty"><div class="e">🎬</div><b>Noch keine Drehs geplant.</b><p>Tipp: Im Lead auf „🎬 Dreh planen“ klicken – Kunde, Adresse und passende Shotlist werden übernommen.</p></div>')}
    ${raw(done.length ? html`<details><summary>${done.length} abgeschlossene Projekte</summary><div class="tiles">${done.map(s => raw(card(s)))}</div></details>` : '')}`;
  el.querySelector('#new').addEventListener('click', async () => {
    const s = await api('/api/shoots/shoots', { method: 'POST', body: { type: el.querySelector('#type').value } });
    openShoot(s.id, () => render(el));
  });
  el.querySelectorAll('[data-open]').forEach(c => c.addEventListener('click', () => openShoot(Number(c.dataset.open), () => render(el))));
}

async function openShoot(id, onClose) {
  document.querySelector('.drawer-bg')?.remove(); document.querySelector('.drawer')?.remove();
  const bg = document.createElement('div'); bg.className = 'drawer-bg';
  const dr = document.createElement('div'); dr.className = 'drawer'; dr.style.width = 'min(680px, 100vw)';
  document.body.append(bg, dr);
  const close = () => { bg.remove(); dr.remove(); onClose?.(); };
  bg.addEventListener('click', close);
  let s = await api('/api/shoots/shoots/' + id);
  const save = async () => { s = await api('/api/shoots/shoots/' + id, { method: 'PUT', body: s }); };
  const checklist = (key, withDue = false) => html`<div data-list="${key}">${s[key].map((it, i) => raw(html`<div class="row" style="flex-wrap:nowrap;margin:4px 0">
      <input type="checkbox" data-i="${i}" ${raw(it.done ? 'checked' : '')}><input data-t="${i}" value="${it.text}" style="flex:1;padding:5px 9px">
      ${raw(withDue ? html`<input type="date" data-due="${i}" value="${it.due || ''}" style="width:150px;padding:5px 9px">` : '')}
      <button class="btn sm ghost icon" data-rm="${i}">✕</button></div>`))}
    <button class="btn sm ghost" data-add="${key}">＋ hinzufügen</button></div>`;
  const draw = async () => {
    dr.innerHTML = html`
      <div class="row"><input id="title" value="${s.title}" style="font-size:20px;font-weight:600;border:0;background:transparent;padding:0;flex:1"><button class="btn sm icon" id="x">✕</button></div>
      <div class="card flat form"><div class="fields">
        <label><span>Art</span><select data-k="type">${Object.entries(META.types).map(([k, l]) => raw(html`<option value="${k}" ${raw(k === s.type ? 'selected' : '')}>${l}</option>`))}</select></label>
        <label><span>Status</span><select data-k="status">${Object.entries(STATUS).map(([k, [l]]) => raw(html`<option value="${k}" ${raw(k === s.status ? 'selected' : '')}>${l}</option>`))}</select></label>
        <label><span>Datum</span><input type="date" data-k="date" value="${s.date || ''}"></label>
        <label><span>Call Time</span><input type="time" data-k="call_time" value="${s.call_time || ''}"></label>
        <label><span>Ende</span><input type="time" data-k="end_time" value="${s.end_time || ''}"></label>
        <label style="grid-column:1/-1"><span>Location</span><input data-k="location" value="${s.location || ''}" placeholder="Adresse – für Karte, Wetter & Golden Hour"></label>
        <label style="grid-column:1/-1"><span>Kontakt</span><input data-k="contact" value="${s.contact || ''}"></label>
        <label style="grid-column:1/-1"><span>Crew</span><input data-k="crew" value="${s.crew || ''}" placeholder="z.B. 2. Kamera, Licht, Make-up"></label>
      </div></div>
      <div class="card flat" id="cond"><span class="spinner"></span> Wetter & Licht …</div>
      <div class="card flat"><h3>🎬 Shotlist</h3>${raw(checklist('shots'))}</div>
      <div class="card flat"><h3>🎒 Equipment</h3>${raw(checklist('gear'))}</div>
      <div class="card flat"><h3>📦 Lieferungen & Deadlines</h3>${raw(checklist('deliverables', true))}</div>
      <div class="card flat form"><label><span>Notizen / Konzept / Moodboard-Links</span><textarea data-k="notes">${s.notes || ''}</textarea></label></div>
      <div class="row"><a class="btn primary" href="/api/shoots/shoots/${id}/callsheet" target="_blank" rel="noopener">Call Sheet ↗︎</a>
        ${raw(s.location ? html`<a class="btn" href="https://www.google.com/maps/search/?api=1&query=${encodeURIComponent(s.location)}" target="_blank" rel="noopener">Karte ↗︎</a>` : '')}
        ${raw(s.lead_id ? html`<a class="btn ghost" href="#/leads" id="to-lead">Zum Lead</a>` : '')}
        <span class="spacer"></span><button class="btn sm ghost" id="del">Löschen</button></div>
      <p class="small muted" style="margin-top:10px">Status „Geliefert“ → im Lead wird automatisch eine Erinnerung gesetzt: nach Testimonial, Google-Bewertung & Empfehlung fragen.</p>`;
    dr.querySelector('#x').addEventListener('click', close);
    dr.querySelector('#title').addEventListener('change', async e => { s.title = e.target.value; await save(); });
    dr.querySelectorAll('[data-k]').forEach(i => i.addEventListener('change', async () => {
      s[i.dataset.k] = i.value; await save();
      if (['date', 'location'].includes(i.dataset.k)) loadCond();
      if (i.dataset.k === 'status') toast(`Status: ${STATUS[i.value][0]}`, 'ok');
    }));
    dr.querySelectorAll('[data-list]').forEach(box => {
      const key = box.dataset.list;
      box.querySelectorAll('[data-i]').forEach(cb => cb.addEventListener('change', async () => { s[key][cb.dataset.i].done = cb.checked; await save(); }));
      box.querySelectorAll('[data-t]').forEach(t => t.addEventListener('change', async () => { s[key][t.dataset.t].text = t.value; await save(); }));
      box.querySelectorAll('[data-due]').forEach(t => t.addEventListener('change', async () => { s[key][t.dataset.due].due = t.value; await save(); }));
      box.querySelectorAll('[data-rm]').forEach(b => b.addEventListener('click', async () => { s[key].splice(b.dataset.rm, 1); await save(); draw(); }));
      box.querySelector('[data-add]').addEventListener('click', async () => { s[key].push({ text: '', done: false, ...(key === 'deliverables' ? { due: s.date } : {}) }); await save(); draw(); });
    });
    dr.querySelector('#del').addEventListener('click', async () => { if (confirm('Dreh löschen?')) { await api('/api/shoots/shoots/' + id, { method: 'DELETE' }); close(); } });
    loadCond();
  };
  const loadCond = async () => {
    const c = await api(`/api/shoots/shoots/${id}/conditions`);
    const sun = c.sun || {}, w = c.weather;
    const box = dr.querySelector('#cond'); if (!box) return;
    box.innerHTML = html`<div class="grid3">
      <div class="stat"><span class="l">Golden Hour Abend</span><span class="v" style="font-size:20px">${sun.golden_evening || '–'}</span><span class="l">Sonnenuntergang ${sun.sunset || '–'} · Blaue Stunde ${sun.blue_hour || '–'}</span></div>
      <div class="stat"><span class="l">Golden Hour Morgen</span><span class="v" style="font-size:20px">${sun.golden_morning || '–'}</span><span class="l">Sonnenaufgang ${sun.sunrise || '–'}</span></div>
      <div class="stat"><span class="l">Wetter</span><span class="v" style="font-size:20px">${w ? w.text : '–'}</span><span class="l">${w ? `${w.tmin}–${w.tmax} °C · Regen ${w.rain} % · Wind ${w.wind} km/h` : (c.weather_note || 'keine Vorhersage verfügbar')}</span></div></div>
      ${raw(w && w.rain >= 50 ? '<p class="small" style="color:var(--orange)">⚠️ Regenrisiko – Indoor-Alternative oder Ausweichtermin einplanen.</p>' : '')}`;
  };
  draw();
}
