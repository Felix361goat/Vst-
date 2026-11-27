// Leads & CRM module page.
import { api, html, raw, toast, copy, fmt, store } from '/static/api.js';

let M = null;                  // meta (stages, checks, areas …)
let kind = store('leads.kind') || 'artist';
let ctxRef = null;
const KINDS = { artist: '🎤 Artists', restaurant: '🍽️ Restaurants' };
const CH = { instagram: 'Instagram', email: 'E-Mail', phone: 'Telefon', visit: 'Besuch', letter: 'Brief', form: 'Kontaktformular' };
const DM_LIMIT = 15;

const stages = () => M.stages[kind] || M.stages.other;
const stageOf = id => stages().find(s => s.id === id) || { label: id, color: 'gray' };
const badge = id => { const s = stageOf(id); return html`<span class="badge ${s.color}">${s.label}</span>`; };
const igUrl = h => `https://www.instagram.com/${h}/`;
const today = () => new Date().toISOString().slice(0, 10);
const rel = d => {
  if (!d) return '–';
  const days = Math.round((new Date(d) - new Date(today())) / 864e5);
  return days === 0 ? 'heute' : days === 1 ? 'morgen' : days === -1 ? 'gestern' : days < 0 ? `vor ${-days} T.` : `in ${days} T.`;
};
const scoreBar = s => html`<span class="score"><span class="b"><i style="width:${s}%;background:${s >= 70 ? 'var(--green)' : s >= 50 ? 'var(--blue)' : 'var(--quaternary)'}"></i></span><span class="small">${s}</span></span>`;

export default {
  async mount(el, ctx) {
    ctxRef = ctx;
    M = await api('/api/leads/meta');
    el.innerHTML = html`
      <div class="row" style="align-items:flex-end">
        <div><h1>Leads & CRM</h1><p class="lead">Finden, anschreiben, nachfassen – alles an einem Ort.</p></div>
        <span class="spacer"></span>
        <div class="tabs" id="kind">${Object.entries(KINDS).map(([k, l]) => raw(html`<button data-k="${k}" class="${k === kind ? 'active' : ''}">${l}</button>`))}</div>
      </div>
      <div class="tabs" id="tabs">
        <button data-t="today">Heute</button><button data-t="board">Pipeline</button><button data-t="list">Liste</button>
        <button data-t="find">Lead-Finder</button><button data-t="templates">Vorlagen</button><button data-t="rules">Regeln & Tipps</button>
      </div>
      <div id="tab"></div>`;
    const tabs = { today: tabToday, board: tabBoard, list: tabList, find: tabFind, templates: tabTemplates, rules: tabRules };
    const show = t => {
      el.querySelectorAll('#tabs button').forEach(b => b.classList.toggle('active', b.dataset.t === t));
      store('leads.tab', t);
      tabs[t](el.querySelector('#tab'));
    };
    el.querySelectorAll('#tabs button').forEach(b => b.addEventListener('click', () => show(b.dataset.t)));
    el.querySelectorAll('#kind button').forEach(b => b.addEventListener('click', () => {
      kind = b.dataset.k; store('leads.kind', kind);
      el.querySelectorAll('#kind button').forEach(x => x.classList.toggle('active', x === b));
      show(store('leads.tab') || 'today');
    }));
    show(store('leads.tab') || 'today');
  },
};

function rerender() { document.querySelector('#tabs button.active')?.click(); }

// ------------------------------------------------------------------ today
async function tabToday(el) {
  const [s, due, best] = await Promise.all([
    api('/api/leads/stats'),
    api(`/api/leads/leads?kind=${kind}&due=1&sort=next&limit=50`),
    api(`/api/leads/leads?kind=${kind}&status=neu,warmup&sort=score&limit=8`),
  ]);
  const tot = s.totals.find(t => t.kind === kind) || { n: 0, ig: 0, em: 0, ph: 0 };
  const dmPct = Math.min(100, (100 * s.dms_today) / DM_LIMIT);
  el.innerHTML = html`
    <div class="grid4" style="margin-top:14px">
      <div class="card stat"><span class="v">${due.total}</span><span class="l">Follow-ups fällig</span></div>
      <div class="card stat"><span class="v">${s.contacted_today}</span><span class="l">Heute kontaktiert · ${s.contacted_week} diese Woche</span></div>
      <div class="card stat"><span class="v">${s.reply_rate == null ? '–' : s.reply_rate + ' %'}</span><span class="l">Antwortquote (${s.replied}/${s.contacted})</span></div>
      <div class="card stat"><span class="v">${fmt(Math.round(s.won_value))} €</span><span class="l">${s.won} gebucht · ${fmt(Math.round(s.pipeline_value))} € in Verhandlung</span></div>
    </div>
    ${raw(kind === 'artist' ? html`<div class="card flat"><div class="row"><b>Instagram-DMs heute</b><span class="spacer"></span><span class="small muted">${s.dms_today} / ~${DM_LIMIT} (sicheres Tageslimit für kleine Accounts)</span></div>
      <div class="progress-line"><div style="width:${dmPct}%;background:${dmPct >= 100 ? 'var(--red)' : 'var(--blue)'}"></div></div></div>` : '')}
    ${raw(tot.n === 0 ? html`<div class="card empty"><div class="e">${kind === 'artist' ? '🎤' : '🍽️'}</div><b>Noch keine ${KINDS[kind].slice(3)}.</b><p>Starte im <a href="#" id="go-find">Lead-Finder</a> – oder füge Instagram-Namen per Copy & Paste ein.</p></div>` : '')}
    <div class="grid2">
      <div class="card"><h3>⏰ Heute nachfassen</h3>${raw(due.items.length ? leadRows(due.items, true) : '<p class="muted small">Nichts fällig. 🎉</p>')}</div>
      <div class="card"><h3>⭐ Beste neue Leads</h3>${raw(best.items.length ? leadRows(best.items, false) : '<p class="muted small">Keine neuen Leads.</p>')}
        <p class="small muted">${tot.n} gesamt · ${tot.ig} mit Instagram · ${tot.em} mit E-Mail · ${tot.ph} mit Telefon</p></div>
    </div>
    ${raw(kind === 'restaurant' ? html`<div class="card"><div class="row"><h3 style="margin:0">🚶 Walk-in-Route für heute</h3><span class="spacer"></span>
      <select id="r-d" style="width:auto"><option value="">Alle Gebiete</option></select>
      <select id="r-n" style="width:auto">${[5, 6, 8, 10].map(n => raw(html`<option ${raw(n === 6 ? 'selected' : '')}>${n}</option>`))}</select><span class="small muted">Stopps</span>
      <button class="btn sm primary" id="r-go">Route planen</button></div>
      <p class="small muted">Persönlich vorbeigehen ist rechtlich der sicherste und erfolgreichste Weg (Di–Do 14:30–17:00). Beste noch nicht kontaktierte Lokale, als kurze Fußroute sortiert.</p>
      <div id="route"></div></div>` : '')}
    <div class="card"><h3>🕘 Letzte Aktivität</h3>${raw(s.recent.length ? html`<div class="timeline">${s.recent.map(a => raw(html`<div class="ev"><a href="#" data-open="${a.id}"><b>${a.name}</b></a> · ${actLabel(a)}${raw(a.text ? html`<div class="small muted">${a.text.slice(0, 140)}</div>` : '')}<div class="t">${a.ts}</div></div>`))}</div>` : '<p class="muted small">Noch nichts passiert.</p>')}</div>`;
  el.querySelector('#go-find')?.addEventListener('click', e => { e.preventDefault(); document.querySelector('#tabs [data-t=find]').click(); });
  wireRows(el);
  if (kind === 'restaurant') wireRoute(el);
}

async function wireRoute(el) {
  const sel = el.querySelector('#r-d');
  const ds = (await api('/api/leads/leads?kind=restaurant&limit=1')).districts;
  ds.forEach(d => sel.add(new Option(d, d, false, d === store('leads.route.d'))));
  el.querySelector('#r-go').addEventListener('click', async () => {
    store('leads.route.d', sel.value);
    const r = await api(`/api/leads/route?district=${encodeURIComponent(sel.value)}&stops=${el.querySelector('#r-n').value}`);
    const box = el.querySelector('#route');
    if (!r.stops.length) { box.innerHTML = '<p class="muted small">Keine offenen Lokale mit Koordinaten in diesem Gebiet.</p>'; return; }
    box.innerHTML = html`<table class="tbl">${r.stops.map((l, i) => raw(html`<tr><td class="c"><span class="badge blue">${i + 1}</span></td>
      <td><a href="#" data-open="${l.id}" class="lead-name">${l.name}</a><div class="lead-sub">${l.address || ''} · ${l.category || ''}</div></td>
      <td>${raw(scoreBar(l.score))}</td><td style="text-align:right"><button class="btn sm" data-visit="${l.id}">🚶 Besucht</button></td></tr>`))}</table>
      <div class="row" style="margin-top:10px"><a class="btn primary" href="${r.maps_url}" target="_blank" rel="noopener">In Google Maps öffnen ↗︎</a>
      <span class="small muted">≈ ${r.km} km zu Fuß · ca. ${r.minutes} Min. inkl. Gesprächen</span></div>`;
    wireRows(box);
    box.querySelectorAll('[data-visit]').forEach(b => b.addEventListener('click', async () => {
      await api(`/api/leads/leads/${b.dataset.visit}/log`, { method: 'POST', body: { type: 'visit', channel: 'visit' } });
      b.textContent = '✓'; b.disabled = true; toast('Besuch eingetragen', 'ok');
    }));
  });
}

const actLabel = a => ({ contact: `✉️ angeschrieben (${CH[a.channel] || a.channel})`, reply: '↩️ hat geantwortet', note: '📝 Notiz', call: '📞 Anruf', visit: '🚶 besucht', letter: '✉️ Brief', status: '🔀 Status', check: '☑️' }[a.type] || a.type);

function leadRows(items, showDue) {
  return html`<table class="tbl">${items.map(l => raw(html`<tr>
    <td><a href="#" data-open="${l.id}" class="lead-name">${l.name}</a><div class="lead-sub">${raw(l.instagram ? html`<span class="ig">@${l.instagram}</span> · ` : '')}${l.district || l.category || ''}</div></td>
    <td>${raw(badge(l.status))}</td>
    <td class="small">${showDue ? rel(l.next_action_date) : raw(scoreBar(l.score))}</td>
    <td style="text-align:right;white-space:nowrap">${raw(quickButtons(l))}</td></tr>`))}</table>`;
}

function quickButtons(l) {
  const parts = [];
  if (l.instagram) parts.push(html`<a class="btn sm icon" href="${igUrl(l.instagram)}" target="_blank" rel="noopener" title="Profil öffnen">↗︎</a>`);
  parts.push(html`<button class="btn sm" data-contact="${l.id}" title="Als kontaktiert markieren">✓ ${l.contact_count ? 'Nachgefasst' : 'Kontaktiert'}</button>`);
  parts.push(html`<button class="btn sm ghost" data-reply="${l.id}" title="Hat geantwortet">↩︎</button>`);
  return parts.join('');
}

function wireRows(el) {
  el.querySelectorAll('[data-open]').forEach(a => a.addEventListener('click', e => { e.preventDefault(); openLead(Number(a.dataset.open)); }));
  el.querySelectorAll('[data-contact]').forEach(b => b.addEventListener('click', async () => {
    const id = b.dataset.contact;
    const channel = kind === 'artist' ? 'instagram' : (prompt('Wie kontaktiert? instagram / visit / letter / email / phone', 'visit') || '').trim();
    if (!channel) return;
    try { await api(`/api/leads/leads/${id}/log`, { method: 'POST', body: { type: channel === 'visit' ? 'visit' : channel === 'letter' ? 'letter' : 'contact', channel } }); toast('Kontakt eingetragen – Follow-up geplant', 'ok'); rerender(); ctxRef.refreshStatus?.(); }
    catch (e) { toast(e.message, 'err'); }
  }));
  el.querySelectorAll('[data-reply]').forEach(b => b.addEventListener('click', async () => {
    await api(`/api/leads/leads/${b.dataset.reply}/log`, { method: 'POST', body: { type: 'reply', channel: kind === 'artist' ? 'instagram' : 'email' } });
    toast('Antwort eingetragen 🎉', 'ok'); rerender();
  }));
}

// ------------------------------------------------------------------ board (kanban)
async function tabBoard(el) {
  const data = await api(`/api/leads/leads?kind=${kind}&limit=600&sort=score`);
  const cols = stages();
  el.innerHTML = html`<p class="small muted" style="margin:10px 0">Karten zwischen Spalten ziehen, um den Status zu ändern. Klick öffnet Details.</p>
    <div class="kanban">${cols.map(s => {
      const items = data.items.filter(l => l.status === s.id);
      return raw(html`<div class="kcol" data-stage="${s.id}"><h4><span><span class="dot" style="background:var(--${s.color === 'gray' ? 'quaternary' : s.color})"></span> ${s.label}</span><span>${items.length}</span></h4>
        ${items.slice(0, 80).map(l => raw(html`<div class="kcard" draggable="true" data-id="${l.id}"><div class="n">${l.starred ? '★ ' : ''}${l.name}</div>
          <div class="m">${raw(l.instagram ? html`<span class="ig">@${l.instagram}</span>` : '')} ${l.district || ''}</div>
          <div class="row small" style="margin-top:4px">${raw(scoreBar(l.score))}<span class="spacer"></span><span class="muted">${l.next_action_date ? '⏰ ' + rel(l.next_action_date) : ''}</span></div></div>`))}
        ${raw(items.length > 80 ? html`<p class="small muted">+ ${items.length - 80} weitere (Liste)</p>` : '')}</div>`);
    })}</div>`;
  let dragId = null;
  el.querySelectorAll('.kcard').forEach(c => {
    c.addEventListener('dragstart', () => { dragId = c.dataset.id; });
    c.addEventListener('click', () => openLead(Number(c.dataset.id)));
  });
  el.querySelectorAll('.kcol').forEach(col => {
    col.addEventListener('dragover', e => { e.preventDefault(); col.classList.add('over'); });
    col.addEventListener('dragleave', () => col.classList.remove('over'));
    col.addEventListener('drop', async e => {
      e.preventDefault(); col.classList.remove('over');
      if (!dragId) return;
      await api(`/api/leads/leads/${dragId}`, { method: 'PATCH', body: { values: { status: col.dataset.stage } } });
      tabBoard(el);
    });
  });
}

// ------------------------------------------------------------------ list
async function tabList(el) {
  const f = { q: '', status: '', district: '', has: '', sort: 'score', ...(store('leads.filter.' + kind) || {}) };
  let offset = 0;
  el.innerHTML = html`
    <div class="card flat filters" style="margin-top:14px">
      <input id="f-q" placeholder="Suchen …" value="${f.q}" style="min-width:200px">
      <select id="f-status"><option value="">Alle Status</option>${stages().map(s => raw(html`<option value="${s.id}" ${raw(f.status === s.id ? 'selected' : '')}>${s.label}</option>`))}</select>
      <select id="f-district"><option value="">Alle Gebiete</option></select>
      <select id="f-has">${[['', 'Alle Kontakte'], ['instagram', 'mit Instagram'], ['email', 'mit E-Mail'], ['phone', 'mit Telefon'], ['website', 'mit Website']].map(([v, l]) => raw(html`<option value="${v}" ${raw(f.has === v ? 'selected' : '')}>${l}</option>`))}</select>
      <select id="f-sort">${[['score', 'Beste zuerst'], ['next', 'Nächste Aktion'], ['name', 'Name A–Z'], ['district', 'Gebiet'], ['created', 'Neueste'], ['updated', 'Zuletzt geändert']].map(([v, l]) => raw(html`<option value="${v}" ${raw(f.sort === v ? 'selected' : '')}>${l}</option>`))}</select>
      <span class="spacer"></span>
      <button class="btn sm" id="add">＋ Neu</button>
      <a class="btn sm ghost" href="/api/leads/export.csv?kind=${kind}">CSV exportieren</a>
    </div>
    <div class="card flat" id="bulkbar" style="display:none"><div class="row"><b id="bulk-n"></b><span class="spacer"></span>
      <select id="bulk-status" style="width:auto"><option value="">Status setzen …</option>${stages().map(s => raw(html`<option value="${s.id}">${s.label}</option>`))}</select>
      <button class="btn sm" data-bulk="star">★ Markieren</button><button class="btn sm" data-bulk="archive">Archivieren</button><button class="btn sm red" data-bulk="delete">Löschen</button></div></div>
    <div class="card" style="padding:8px 8px 4px"><div style="overflow-x:auto"><table class="tbl" id="tbl"></table></div>
      <div class="row" style="justify-content:center;padding:10px"><button class="btn sm ghost" id="more">Mehr laden</button></div></div>`;
  const tbl = el.querySelector('#tbl');
  const sel = new Set();
  const load = async (append = false) => {
    const qs = new URLSearchParams({ kind, ...f, limit: 100, offset }).toString();
    const d = await api('/api/leads/leads?' + qs);
    const dsel = el.querySelector('#f-district');
    if (dsel.options.length === 1) d.districts.forEach(x => dsel.add(new Option(x, x, false, x === f.district)));
    const rows = d.items.map(l => html`<tr>
      <td><input type="checkbox" data-sel="${l.id}" ${raw(sel.has(l.id) ? 'checked' : '')}></td>
      <td><a href="#" data-open="${l.id}" class="lead-name">${l.starred ? '★ ' : ''}${l.name}</a><div class="lead-sub">${l.category || ''}</div></td>
      <td>${raw(l.instagram ? html`<a class="ig" href="${igUrl(l.instagram)}" target="_blank" rel="noopener">@${l.instagram}</a>` : '<span class="muted">–</span>')}</td>
      ${raw(kind !== 'artist' ? html`<td class="small">${raw(l.email ? html`<a href="mailto:${l.email}">${l.email}</a>` : '–')}</td><td class="small">${raw(l.phone ? html`<a href="tel:${l.phone}">${l.phone}</a>` : '–')}</td>` : html`<td class="small">${l.followers ? fmt(l.followers) : '–'}</td>`)}
      <td class="small">${l.district || '–'}</td><td>${raw(badge(l.status))}</td><td>${raw(scoreBar(l.score))}</td>
      <td class="small">${l.last_contact_at ? rel(l.last_contact_at) : '–'}</td><td class="small">${rel(l.next_action_date)}</td>
      <td style="white-space:nowrap;text-align:right">${raw(quickButtons(l))}</td></tr>`).join('');
    const head = html`<tr><th><input type="checkbox" id="sel-all"></th><th>Name (${d.total})</th><th>Instagram</th>${raw(kind !== 'artist' ? '<th>E-Mail</th><th>Telefon</th>' : '<th>Follower</th>')}<th>Gebiet</th><th>Status</th><th>Score</th><th>Letzter Kontakt</th><th>Nächste Aktion</th><th></th></tr>`;
    tbl.innerHTML = append ? tbl.innerHTML + rows : head + (rows || `<tr><td colspan="11" class="empty">Keine Treffer.</td></tr>`);
    el.querySelector('#more').style.display = offset + 100 < d.total ? '' : 'none';
    wireRows(tbl);
    tbl.querySelectorAll('[data-sel]').forEach(cb => cb.addEventListener('change', () => { cb.checked ? sel.add(Number(cb.dataset.sel)) : sel.delete(Number(cb.dataset.sel)); bulkbar(); }));
    tbl.querySelector('#sel-all')?.addEventListener('change', e => { tbl.querySelectorAll('[data-sel]').forEach(cb => { cb.checked = e.target.checked; cb.dispatchEvent(new Event('change')); }); });
  };
  const bulkbar = () => { el.querySelector('#bulkbar').style.display = sel.size ? '' : 'none'; el.querySelector('#bulk-n').textContent = `${sel.size} ausgewählt`; };
  const apply = () => { store('leads.filter.' + kind, f); offset = 0; load(); };
  let t;
  el.querySelector('#f-q').addEventListener('input', e => { f.q = e.target.value; clearTimeout(t); t = setTimeout(apply, 300); });
  for (const k of ['status', 'district', 'has', 'sort']) el.querySelector('#f-' + k).addEventListener('change', e => { f[k] = e.target.value; apply(); });
  el.querySelector('#more').addEventListener('click', () => { offset += 100; load(true); });
  el.querySelector('#add').addEventListener('click', () => newLeadDialog());
  el.querySelector('#bulk-status').addEventListener('change', async e => { if (!e.target.value) return; await api('/api/leads/bulk', { method: 'POST', body: { ids: [...sel], action: 'status', value: e.target.value } }); sel.clear(); bulkbar(); load(); });
  el.querySelectorAll('[data-bulk]').forEach(b => b.addEventListener('click', async () => {
    if (b.dataset.bulk === 'delete' && !confirm(`${sel.size} Leads endgültig löschen?`)) return;
    await api('/api/leads/bulk', { method: 'POST', body: { ids: [...sel], action: b.dataset.bulk } }); sel.clear(); bulkbar(); load();
  }));
  load();
}

async function newLeadDialog() {
  const name = prompt(kind === 'artist' ? 'Artist-Name oder @instagram:' : 'Name des Lokals:');
  if (!name) return;
  const ig = name.includes('@') || name.includes('instagram.com') ? name : '';
  try {
    const r = await api('/api/leads/leads', { method: 'POST', body: { kind, name: ig ? name.replace(/.*instagram\.com\//, '').replace('@', '').replace(/\/.*/, '') : name, instagram: ig || null } });
    if (r.id) openLead(r.id); else toast('Gibt es schon – ergänzt', 'ok');
    rerender();
  } catch (e) { toast(e.message, 'err'); }
}

// ------------------------------------------------------------------ lead drawer
async function openLead(id) {
  document.querySelector('.drawer-bg')?.remove(); document.querySelector('.drawer')?.remove();
  const bg = document.createElement('div'); bg.className = 'drawer-bg';
  const d = document.createElement('div'); d.className = 'drawer';
  document.body.append(bg, d);
  const close = () => { bg.remove(); d.remove(); rerender(); };
  bg.addEventListener('click', close);
  const onKey = e => { if (e.key === 'Escape') { close(); document.removeEventListener('keydown', onKey); } };
  document.addEventListener('keydown', onKey);
  const templates = await api(`/api/leads/templates?kind=${kind}`);
  const render = async () => {
    const l = await api(`/api/leads/leads/${id}`);
    const k = l.kind;
    const checks = M.checks[k] || [], quals = M.qualifiers[k] || [], prices = M.prices[k] || [];
    const ex = l.extra || {};
    const links = [
      l.instagram && html`<a class="btn sm" href="${igUrl(l.instagram)}" target="_blank" rel="noopener">Instagram ↗︎</a>`,
      l.email && html`<a class="btn sm" href="mailto:${l.email}">E-Mail</a>`,
      l.phone && html`<a class="btn sm" href="tel:${l.phone}">Anrufen</a>`,
      l.website && html`<a class="btn sm" href="${l.website.startsWith('http') ? l.website : 'https://' + l.website}" target="_blank" rel="noopener">Website ↗︎</a>`,
      l.lat && html`<a class="btn sm" href="https://www.google.com/maps/search/?api=1&query=${encodeURIComponent(l.name + ' ' + (l.address || ''))}" target="_blank" rel="noopener">Maps ↗︎</a>`,
      ...['youtube', 'spotify', 'soundcloud', 'tiktok'].filter(x => ex[x]).map(x => html`<a class="btn sm ghost" href="${ex[x]}" target="_blank" rel="noopener">${x} ↗︎</a>`),
      !l.instagram && k === 'artist' && html`<a class="btn sm ghost" href="https://www.google.com/search?q=${encodeURIComponent(l.name + ' rapper instagram')}" target="_blank" rel="noopener">IG suchen ↗︎</a>`,
      !l.instagram && k !== 'artist' && html`<a class="btn sm ghost" href="https://www.google.com/search?q=${encodeURIComponent(l.name + ' ' + (l.district || '') + ' instagram')}" target="_blank" rel="noopener">IG suchen ↗︎</a>`,
    ].filter(Boolean);
    d.innerHTML = html`
      <div class="row"><h2 style="margin:0">${l.name}</h2><span class="spacer"></span>
        <button class="btn sm icon" id="star" title="Markieren">${l.starred ? '★' : '☆'}</button><button class="btn sm icon" id="x">✕</button></div>
      <p class="small muted" style="margin:4px 0 12px">${l.category || ''} ${l.district ? '· ' + l.district : ''} · Quelle: ${l.source || '–'} · Score ${l.score}</p>
      <div class="row">${raw(links.join(''))}</div>
      ${raw(l.flags.do_not_contact ? '<div class="card error small">⛔ Sperrliste – diese Person/dieses Lokal will nicht kontaktiert werden.</div>' : '')}
      ${raw(ex.bio ? html`<div class="card flat small" style="white-space:pre-wrap">${ex.bio}</div>` : '')}

      <div class="card flat"><div class="row"><b>Status</b><span class="spacer"></span>
        <select id="status" style="width:auto">${stages().map(s => raw(html`<option value="${s.id}" ${raw(s.id === l.status ? 'selected' : '')}>${s.label}</option>`))}</select></div>
        <div class="row small muted" style="margin-top:8px">Kontakte: ${l.contact_count || 0} · erster: ${l.first_contact_at || '–'} · Antwort: ${l.replied_at || '–'}</div>
        <div class="row" style="margin-top:8px"><span class="small">Nächste Aktion</span><input type="date" id="next" value="${l.next_action_date || ''}" style="width:auto"><span class="small muted">${rel(l.next_action_date)}</span></div></div>

      <div class="card flat"><h3>Nachricht</h3>
        <div class="row"><select id="tpl">${templates.filter(t => t.kind === k).map(t => raw(html`<option value="${t.id}">${CH[t.channel] || t.channel} · ${t.name}</option>`))}</select></div>
        <div id="tpl-warn" class="small" style="margin:8px 0"></div>
        <input id="tpl-subj" placeholder="Betreff" style="display:none;margin-bottom:6px">
        <textarea id="tpl-body" style="min-height:150px"></textarea>
        <div id="tpl-ph" class="small" style="margin:6px 0"></div>
        <div class="row"><button class="btn sm" id="tpl-copy">Kopieren</button>
          <button class="btn sm primary" id="tpl-send">Kopieren & als kontaktiert markieren</button></div></div>

      <div class="card flat"><h3>Schnell eintragen</h3><div class="row">
        <button class="btn sm" data-log="contact">✉️ Angeschrieben</button><button class="btn sm" data-log="reply">↩️ Hat geantwortet</button>
        <button class="btn sm" data-log="visit">🚶 Besucht</button><button class="btn sm" data-log="letter">📮 Brief</button><button class="btn sm" data-log="call">📞 Telefonat</button>
        <button class="btn sm ghost" data-log="note">📝 Notiz</button></div>
        <textarea id="log-text" placeholder="Was wurde besprochen? (optional)" style="min-height:60px;margin-top:8px"></textarea></div>

      <div class="card flat"><h3>Checkliste</h3><div class="checkgrid">${checks.map(c => raw(html`<label class="switch small"><input type="checkbox" data-flag="${c.id}" ${raw(l.flags[c.id] ? 'checked' : '')}> ${c.label}</label>`))}</div></div>

      <div class="card flat"><h3>Qualifizierung <span class="small muted">(erhöht den Score)</span></h3><div class="checkgrid">
        ${quals.map(q => raw(html`<label class="switch small"><input type="checkbox" data-flag="${q.id}" ${raw(l.flags[q.id] ? 'checked' : '')}> ${q.label} <span class="muted">+${q.points}</span></label>`))}</div></div>

      <div class="card flat form"><h3>Daten</h3><div class="fields">
        ${['name', 'instagram', 'email', 'phone', 'website', 'address', 'district', 'category'].map(f => raw(html`<label><span>${{ name: 'Name', instagram: 'Instagram', email: 'E-Mail', phone: 'Telefon', website: 'Website', address: 'Adresse', district: 'Gebiet', category: 'Kategorie' }[f]}</span><input data-f="${f}" value="${l[f] || ''}"></label>`))}
        ${raw(k === 'artist' ? html`<label><span>IG-Follower</span><input data-f="followers" value="${l.followers || ''}"></label><label><span>Spotify Hörer/Monat</span><input data-f="monthly_listeners" value="${l.monthly_listeners || ''}"></label>` : '')}
        <label><span>Ansprache</span><select data-f="tone"><option value="du" ${raw(l.tone !== 'Sie' ? 'selected' : '')}>du</option><option value="Sie" ${raw(l.tone === 'Sie' ? 'selected' : '')}>Sie</option></select></label>
        <label><span>Deal-Wert geschätzt (€)</span><input data-f="deal_value_est" value="${l.deal_value_est || ''}"></label>
        <label><span>Deal-Wert final (€)</span><input data-f="deal_value_final" value="${l.deal_value_final || ''}"></label>
        <label><span>Grund (verloren)</span><select data-f="lost_reason">${['', 'Budget', 'Hat schon Videograf', 'Kein Bedarf', 'Keine Antwort', 'Sonstiges'].map(r => raw(html`<option ${raw(l.lost_reason === r ? 'selected' : '')}>${r}</option>`))}</select></label>
      </div>
      <p class="small muted">Preisanker Wien (geschätzt): ${prices.map(p => `${p[0]} ${p[1]}–${p[2]} €`).join(' · ')}</p>
      <label><span>Notizen</span><textarea data-f="notes" style="min-height:80px">${l.notes || ''}</textarea></label>
      <div class="row"><button class="btn sm primary" id="save">Speichern</button><span class="spacer"></span>
        <button class="btn sm red" id="block">${l.flags.do_not_contact ? 'Von Sperrliste nehmen' : 'Sperrliste (will keinen Kontakt)'}</button>
        <button class="btn sm ghost" id="del">Löschen</button></div></div>

      <div class="card flat"><h3>Verlauf</h3>${raw(l.activities.length ? html`<div class="timeline">${l.activities.map(a => raw(html`<div class="ev">${actLabel(a)}${raw(a.text ? html`<div class="small">${a.text}</div>` : '')}<div class="t">${a.ts}</div></div>`))}</div>` : '<p class="small muted">Noch keine Einträge.</p>')}</div>`;

    d.querySelector('#x').addEventListener('click', close);
    d.querySelector('#star').addEventListener('click', async () => { await api(`/api/leads/leads/${id}`, { method: 'PATCH', body: { values: { starred: l.starred ? 0 : 1 } } }); render(); });
    d.querySelector('#status').addEventListener('change', async e => { await api(`/api/leads/leads/${id}`, { method: 'PATCH', body: { values: { status: e.target.value } } }); toast('Status geändert', 'ok'); render(); });
    d.querySelector('#next').addEventListener('change', async e => { await api(`/api/leads/leads/${id}`, { method: 'PATCH', body: { values: { next_action_date: e.target.value || null } } }); render(); });
    d.querySelectorAll('[data-flag]').forEach(cb => cb.addEventListener('change', async () => { await api(`/api/leads/leads/${id}`, { method: 'PATCH', body: { flags: { [cb.dataset.flag]: cb.checked } } }); render(); }));
    d.querySelector('#save').addEventListener('click', async () => {
      const values = {}; d.querySelectorAll('[data-f]').forEach(i => { values[i.dataset.f] = i.value === '' ? null : i.value; });
      await api(`/api/leads/leads/${id}`, { method: 'PATCH', body: { values } }); toast('Gespeichert', 'ok'); render();
    });
    d.querySelector('#block').addEventListener('click', async () => {
      const on = !l.flags.do_not_contact;
      await api(`/api/leads/leads/${id}`, { method: 'PATCH', body: { flags: { do_not_contact: on } } });
      if (on) await api(`/api/leads/leads/${id}/log`, { method: 'POST', body: { type: 'note', channel: 'other', text: 'Auf Sperrliste gesetzt (Widerspruch / kein Interesse)' } });
      render();
    });
    d.querySelector('#del').addEventListener('click', async () => { if (confirm('Lead löschen?')) { await api(`/api/leads/leads/${id}`, { method: 'DELETE' }); close(); } });
    d.querySelectorAll('[data-log]').forEach(b => b.addEventListener('click', async () => {
      const type = b.dataset.log;
      const channel = type === 'contact' ? (d.querySelector('#tpl').selectedOptions[0]?.textContent.split(' · ')[0] || 'Instagram') : type;
      const chKey = Object.keys(CH).find(x => CH[x] === channel) || channel;
      try {
        await api(`/api/leads/leads/${id}/log`, { method: 'POST', body: { type, channel: chKey, text: d.querySelector('#log-text').value } });
        toast('Eingetragen', 'ok'); render();
      } catch (e) { toast(e.message, 'err'); }
    }));
    // template rendering
    const tplSel = d.querySelector('#tpl');
    const pick = store('leads.tpl.' + k);
    if (pick && [...tplSel.options].some(o => o.value == pick)) tplSel.value = pick;
    const showTpl = async () => {
      if (!tplSel.value) return;
      store('leads.tpl.' + k, tplSel.value);
      const r = await api(`/api/leads/leads/${id}/render/${tplSel.value}`);
      d.querySelector('#tpl-body').value = r.body;
      const subj = d.querySelector('#tpl-subj'); subj.style.display = r.subject ? '' : 'none'; subj.value = r.subject;
      d.querySelector('#tpl-warn').innerHTML = r.compliance ? html`<div class="card ${r.channel === 'email' || r.channel === 'phone' ? 'reco' : 'hint'}" style="margin:0;padding:10px 12px">${r.compliance}</div>` : '';
      d.querySelector('#tpl-ph').innerHTML = r.open_placeholders.length ? html`<span style="color:var(--orange)">Noch ausfüllen: ${r.open_placeholders.map(p => '{' + p + '}').join(', ')}</span> – persönliche Details machen den Unterschied.` : '';
    };
    tplSel.addEventListener('change', showTpl); showTpl();
    d.querySelector('#tpl-body').addEventListener('input', e => {
      const left = [...new Set((e.target.value.match(/\{(\w+)\}/g) || []))];
      d.querySelector('#tpl-ph').innerHTML = left.length ? html`<span style="color:var(--orange)">Noch ausfüllen: ${left.join(', ')}</span>` : '<span class="pos">✓ Alles ausgefüllt</span>';
    });
    const fullText = () => { const s = d.querySelector('#tpl-subj').value; return (s ? `Betreff: ${s}\n\n` : '') + d.querySelector('#tpl-body').value; };
    d.querySelector('#tpl-copy').addEventListener('click', () => copy(fullText()));
    d.querySelector('#tpl-send').addEventListener('click', async () => {
      if (/\{\w+\}/.test(d.querySelector('#tpl-body').value) && !confirm('Es sind noch Platzhalter wie {song} drin. Trotzdem?')) return;
      await copy(fullText());
      const t = templates.find(x => x.id == tplSel.value);
      try {
        await api(`/api/leads/leads/${id}/log`, { method: 'POST', body: { type: t.channel === 'visit' ? 'visit' : 'contact', channel: t.channel, text: `${t.name}: ${d.querySelector('#tpl-body').value.slice(0, 300)}` } });
        if (t.channel === 'instagram' && l.instagram) window.open(igUrl(l.instagram), '_blank', 'noopener');
        if (t.channel === 'email' && l.email) location.href = `mailto:${l.email}?subject=${encodeURIComponent(d.querySelector('#tpl-subj').value)}&body=${encodeURIComponent(d.querySelector('#tpl-body').value)}`;
        toast('Kopiert & eingetragen – Follow-up geplant', 'ok'); render();
      } catch (e) { toast(e.message, 'err'); }
    });
  };
  render();
}

// ------------------------------------------------------------------ finder
function tabFind(el) {
  const groups = {};
  M.areas.forEach(a => (groups[a.group] ||= []).push(a));
  const savedAreas = new Set(store('leads.areas') || ['moedling:Mödling']);
  el.innerHTML = kind === 'artist' ? html`
    <div class="card"><h3>🎤 Rapper & Artists aus Wien / Mödling finden</h3>
      <p class="small muted">Instagram hat keine offene Suche – das Dashboard sammelt öffentliche Profile aus Musik-Datenbanken und YouTube-Kanälen und übernimmt die dort verlinkten Instagram-Namen.</p>
      <div class="row" style="gap:18px;margin:10px 0">
        <label class="switch"><input type="checkbox" data-src="musicbrainz" checked> MusicBrainz <span class="small muted">(gratis)</span></label>
        <label class="switch"><input type="checkbox" data-src="wikidata" checked> Wikidata <span class="small muted">(gratis, eher bekannte Acts)</span></label>
        <label class="switch"><input type="checkbox" data-src="youtube" ${raw(M.has_youtube ? '' : 'disabled')}> YouTube-Kanäle <span class="small muted">${M.has_youtube ? '(Newcomer! ~8 Suchen)' : '(braucht YouTube-Key)'}</span></label>
      </div>
      <div class="row" style="gap:18px"><label class="switch"><input type="checkbox" data-reg="wien" checked> Wien</label><label class="switch"><input type="checkbox" data-reg="moedling" checked> Bezirk Mödling</label><label class="switch"><input type="checkbox" data-reg="oesterreich"> Ganz Österreich</label></div>
      <details><summary>YouTube-Suchbegriffe anpassen</summary><textarea id="ytq" style="min-height:90px">wien rap\nrap aus wien\nwiener rap musikvideo\nmödling rap\nwien drill\ndeutschrap wien\nfavoriten rap\nösterreich rap neu</textarea></details>
      <div class="row" style="margin-top:12px"><button class="btn primary" id="run-a">Artists suchen</button><span class="small muted">MusicBrainz braucht ~1 Sek. pro Artist – läuft im Hintergrund.</span></div>
      <div id="job"></div></div>
    ${raw(pasteCard())}
    <div class="card"><h3>💡 Wo du die meisten Newcomer findest</h3><ul class="small">
      <li>Hashtags: #wienrap #austrorap #wienerrap #viennarap #1100wien #ösirap #mödling – Profile hier per Copy & Paste einfügen.</li>
      <li>Von jedem guten Lead aus: „Vorgeschlagene Konten“, Features, Follower von lokalen Beatmakern & Studios.</li>
      <li>Open Mics / Cyphers in Wien: vor Ort Kurz-Clips drehen = Content + Kontakt.</li>
      <li>Dein Type-Beat-Kanal: Käufer & Kommentare aus Österreich sind warme Leads – „Musikvideo? DM“ in jede Beschreibung.</li>
      <li>FM4 Soundpark (Hip-Hop-Neuzugänge), The Message Magazine, Spotify-Playlists „Austro Rap“ / „Wien Rap“.</li></ul></div>`
    : html`
    <div class="card"><h3>🍽️ Lokale finden (OpenStreetMap)</h3>
      <p class="small muted">Name, Adresse, Telefon, Website, Instagram & Küche – gratis. Danach optional Impressum/Kontakt-Seiten nach E-Mail & Instagram durchsuchen.</p>
      ${Object.entries(groups).map(([g, list]) => raw(html`<h4 style="margin:14px 0 6px">${g}</h4><div class="row" style="gap:4px 14px">${list.map(a => raw(html`<label class="switch small"><input type="checkbox" data-area="${a.id}" ${raw(savedAreas.has(a.id) ? 'checked' : '')}> ${a.label}</label>`))}</div>`))}
      <h4 style="margin:14px 0 6px">Art</h4><div class="row" style="gap:4px 14px">${Object.entries(M.types).map(([t, l]) => raw(html`<label class="switch small"><input type="checkbox" data-type="${t}" ${raw(['restaurant', 'cafe'].includes(t) ? 'checked' : '')}> ${l}</label>`))}</div>
      <div class="row" style="margin-top:14px"><label class="switch"><input type="checkbox" id="enrich" checked> Websites nach E-Mail & Instagram durchsuchen</label><span class="spacer"></span><button class="btn primary" id="run-r">Lokale suchen</button></div>
      <div id="job"></div></div>
    <div class="card"><h3>🔎 Fehlende Kontaktdaten ergänzen</h3><p class="small muted">Durchsucht Website + Impressum/Kontakt-Seite der Lokale ohne E-Mail oder Instagram (max. 150 pro Lauf, beste Leads zuerst).</p>
      <button class="btn" id="run-e">Kontaktdaten ergänzen</button><div id="job2"></div></div>
    ${raw(pasteCard())}
    <div class="card"><h3>💡 Weitere Quellen für Lokale</h3><ul class="small">
      <li>Neueröffnungen: Falter, Falstaff, 1000things, wien.info, #neueröffnungwien – die brauchen jetzt Content.</li>
      <li>Wolt / Lieferando / Foodora: „Neu“-Filter, Lokale mit schlechten Fotos → Qualifizierung „Wenige Fotos“ anhaken.</li>
      <li>Google Maps: hohe Bewertung, aber wenige Fotos vom Inhaber.</li>
      <li>Netzwerk: Webdesigner, Interior-Designer, Kaffeeröster & Großhändler empfehlen dich weiter.</li></ul></div>`;
  el.querySelectorAll('[data-area]').forEach(cb => cb.addEventListener('change', () => store('leads.areas', [...el.querySelectorAll('[data-area]:checked')].map(x => x.dataset.area))));
  el.querySelector('#run-a')?.addEventListener('click', async () => {
    const body = { sources: [...el.querySelectorAll('[data-src]:checked')].map(x => x.dataset.src), regions: [...el.querySelectorAll('[data-reg]:checked')].map(x => x.dataset.reg), youtube_queries: el.querySelector('#ytq').value.split('\n').map(s => s.trim()).filter(Boolean) };
    runJob(el.querySelector('#job'), '/api/leads/find/artists', body);
  });
  el.querySelector('#run-r')?.addEventListener('click', () => {
    const areas = [...el.querySelectorAll('[data-area]:checked')].map(x => x.dataset.area);
    if (areas.includes('wien') && !confirm('Ganz Wien sind ~6.000 Lokale. Lieber einzelne Bezirke wählen? (OK = trotzdem alle)')) return;
    runJob(el.querySelector('#job'), '/api/leads/find/restaurants', { areas, types: [...el.querySelectorAll('[data-type]:checked')].map(x => x.dataset.type), enrich: el.querySelector('#enrich').checked });
  });
  el.querySelector('#run-e')?.addEventListener('click', () => runJob(el.querySelector('#job2'), '/api/leads/enrich', { limit: 150 }));
  wirePaste(el);
}

function pasteCard() {
  return html`<div class="card"><h3>📋 Instagram-Profile einfügen</h3>
    <p class="small muted">Profile, die du beim Scrollen findest: Links oder @namen einfügen – einer pro Zeile, optional „Name – @handle“.</p>
    <textarea id="paste" placeholder="@beispiel.rapper\nhttps://www.instagram.com/anderer_artist/\nMC Muster – @mc.muster" style="min-height:110px"></textarea>
    <div class="row"><input id="paste-d" placeholder="Gebiet (optional, z.B. Favoriten)" style="max-width:260px"><button class="btn" id="paste-go">Hinzufügen</button>
      <span class="spacer"></span><label class="btn sm ghost" style="cursor:pointer">CSV importieren<input type="file" id="csv" accept=".csv" hidden></label></div></div>`;
}

function wirePaste(el) {
  el.querySelector('#paste-go').addEventListener('click', async () => {
    const r = await api('/api/leads/import/paste', { method: 'POST', body: { text: el.querySelector('#paste').value, kind, district: el.querySelector('#paste-d').value || null } });
    toast(`${r.new} neu · ${r.updated} ergänzt · ${r.same} schon da${r.blocked ? ' · ' + r.blocked + ' gesperrt' : ''}${r.invalid ? ' · ' + r.invalid + ' ungültig' : ''}`, 'ok', 5000);
    el.querySelector('#paste').value = '';
  });
  el.querySelector('#csv').addEventListener('change', async e => {
    const fd = new FormData(); fd.append('file', e.target.files[0]);
    const r = await api(`/api/leads/import/csv?kind=${kind}`, { method: 'POST', form: fd });
    toast(`CSV: ${r.new} neu · ${r.updated} ergänzt · ${r.same} schon da`, 'ok', 5000);
  });
}

async function runJob(box, url, body) {
  try {
    const { job } = await api(url, { method: 'POST', body });
    box.innerHTML = html`<div class="progress-line"><div style="width:2%"></div></div><div class="row small"><span id="jm" class="muted">Startet …</span><span class="spacer"></span><button class="btn sm ghost" id="jc">Abbrechen</button></div>`;
    box.querySelector('#jc').addEventListener('click', () => api('/api/core/jobs/' + job, { method: 'DELETE' }));
    for (;;) {
      await new Promise(r => setTimeout(r, 1200));
      const j = await api('/api/core/jobs/' + job);
      const pct = j.total ? Math.round((100 * j.done) / j.total) : 10;
      box.querySelector('.progress-line div').style.width = Math.max(3, pct) + '%';
      box.querySelector('#jm').textContent = `${j.message || ''} ${j.total ? `(${j.done}/${j.total})` : ''} · ${j.seconds}s`;
      if (j.status !== 'running') {
        if (j.status === 'error') { box.innerHTML = html`<div class="card error small">${j.error}</div>`; return; }
        const r = j.result || {};
        const parts = [`<b>${r.new ?? 0} neue Leads</b>`, `${r.updated ?? 0} ergänzt`];
        if (r.with_instagram != null) parts.push(`${r.with_instagram} mit Instagram`);
        for (const k of ['email', 'instagram', 'phone']) if (r['found_' + k] != null) parts.push(`${r['found_' + k]}× ${k} gefunden`);
        box.innerHTML = html`<div class="card hint small">✓ Fertig: ${raw(parts.join(' · '))}${raw((r.errors || []).length ? html`<div class="neg">${r.errors.join(' | ')}</div>` : '')}
          <div class="row" style="margin-top:8px"><button class="btn sm primary" id="to-list">Zur Liste</button></div></div>`;
        box.querySelector('#to-list').addEventListener('click', () => document.querySelector('#tabs [data-t=list]').click());
        ctxRef.refreshStatus?.();
        return;
      }
    }
  } catch (e) { box.innerHTML = html`<div class="card error small">${e.message}</div>`; }
}

// ------------------------------------------------------------------ templates
async function tabTemplates(el) {
  const list = await api(`/api/leads/templates?kind=${kind}`);
  el.innerHTML = html`<p class="small muted" style="margin:12px 0">Platzhalter: {name} {vorname} {bezirk} {mein_name} {mein_bezirk} {mein_telefon} {portfolio} {monat} – alles andere in {klammern} füllst du pro Lead aus (z.B. {song}). Deine Daten unter Einstellungen.</p>
    ${list.map(t => raw(html`<div class="card form" data-id="${t.id}"><div class="fields">
      <label><span>Name</span><input data-k="name" value="${t.name}"></label>
      <label><span>Kanal</span><select data-k="channel">${Object.entries(CH).map(([v, l]) => raw(html`<option value="${v}" ${raw(v === t.channel ? 'selected' : '')}>${l}</option>`))}</select></label>
      <label><span>Betreff (nur E-Mail)</span><input data-k="subject" value="${t.subject || ''}"></label></div>
      <label style="margin-top:10px"><span>Text</span><textarea data-k="body" style="min-height:120px">${t.body}</textarea></label>
      <div class="row"><button class="btn sm primary" data-save>Speichern</button><button class="btn sm ghost" data-del>Löschen</button></div></div>`))}
    <button class="btn" id="add-t">＋ Neue Vorlage</button>`;
  el.querySelectorAll('[data-id]').forEach(card => {
    const get = () => Object.fromEntries([...card.querySelectorAll('[data-k]')].map(i => [i.dataset.k, i.value]));
    card.querySelector('[data-save]').addEventListener('click', async () => { await api('/api/leads/templates/' + card.dataset.id, { method: 'PUT', body: { kind, ...get() } }); toast('Vorlage gespeichert', 'ok'); });
    card.querySelector('[data-del]').addEventListener('click', async () => { if (confirm('Vorlage löschen?')) { await api('/api/leads/templates/' + card.dataset.id, { method: 'DELETE' }); tabTemplates(el); } });
  });
  el.querySelector('#add-t').addEventListener('click', async () => { await api('/api/leads/templates', { method: 'POST', body: { kind, channel: kind === 'artist' ? 'instagram' : 'visit', name: 'Neue Vorlage', subject: '', body: 'Hallo {vorname}, …' } }); tabTemplates(el); });
}

// ------------------------------------------------------------------ rules
function tabRules(el) {
  const c = M.compliance;
  el.innerHTML = html`
    <div class="card reco"><h3>⚖️ Rechtliches in Österreich – kurz & ehrlich</h3>
      <p class="small">Keine Rechtsberatung, aber das Wichtigste (§174 TKG 2021, DSGVO, Stand 2026):</p>
      <ul class="small"><li>${c.email}</li><li>${c.phone}</li><li>${c.instagram}</li><li>${c.visit}</li><li>${c.letter}</li><li>${c.gdpr}</li>
      <li>Vor jeder Werbe-Mail: Adresse gegen die <a href="https://www.rtr.at/TKP/was_wir_tun/telekommunikation/konsumentenservice/ecg-liste/ECG-Liste.de.html" target="_blank" rel="noopener">ECG-Liste der RTR</a> prüfen.</li></ul></div>
    <div class="grid2">
      <div class="card"><h3>🎤 Artists – Ablauf</h3><ol class="small">
        <li><b>Warm-up:</b> folgen, 2–3 Posts liken, 1 echter Kommentar / Story-Reaktion.</li>
        <li><b>Tag 0:</b> persönliche DM mit Song-Bezug – <i>kein Link</i> in der ersten Nachricht.</li>
        <li><b>Tag 3–4:</b> Follow-up mit Portfolio-Link.</li><li><b>Tag 10:</b> Story-Reaktion, kein Pitch.</li>
        <li><b>Neuer Release:</b> Glückwunsch + konkretes Angebot (Visualizer + 3 Reels).</li>
        <li>Max. 2 Pitch-Nachrichten ohne Antwort, dann 60–90 Tage Pause.</li>
        <li>Beste Zeit: abends 18–23 Uhr & Wochenende. ~10–15 neue DMs pro Tag.</li></ol>
        <p class="small muted">Beste Zielgruppe: 2k–30k Follower, 1k–50k Spotify-Hörer, aktueller Release, schwache Videos.</p></div>
      <div class="card"><h3>🍽️ Restaurants – Ablauf</h3><ol class="small">
        <li><b>Vorbeigehen</b> (Di–Do 14:30–17:00) mit Handy-Reel + Visitenkarte – sicherster & stärkster Weg.</li>
        <li>Dort fragen: „Darf ich Ihnen Beispiele per Mail schicken?“ → <b>Einwilligung</b> notieren (Notiz im Lead).</li>
        <li><b>Tag 4:</b> Mail mit 1 Beispielbild ihres Gerichts.</li><li><b>Tag 10:</b> zweiter Besuch / Gratis-Testbild anbieten.</li>
        <li><b>Tag 30:</b> letzter Kontakt, dann 90 Tage Pause.</li>
        <li>Alternativ: persönlicher Brief mit QR-Code zum Showreel.</li></ol>
        <p class="small muted">Beste Ziele: Neueröffnungen, Top-Bewertungen mit schlechten Fotos, €€–€€€, Lieferplattformen.</p></div>
    </div>
    <div class="card"><h3>💶 Preisanker Wien 2026 (Schätzung)</h3><table class="tbl">${[...M.prices.artist, ...M.prices.restaurant].map(p => raw(html`<tr><td>${p[0]}</td><td>${p[1]}–${p[2]} €</td></tr>`))}</table>
      <p class="small muted">50 % Anzahlung. Bundle-Idee für Rapper: Beat-Lizenz von deinem Type-Beat-Kanal + Musikvideo.</p></div>`;
}
