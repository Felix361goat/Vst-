// Shell: builds the nav from /api/core/modules and loads each module page on demand (#/<id>).
import { api, html, raw, toast, fmt, store } from './api.js';

const view = document.getElementById('view');
const nav = document.getElementById('nav');
let modules = [];

const corePages = {
  home: { id: 'home', name: 'Übersicht', icon: '🏠', mount: mountHome },
  settings: { id: 'settings', name: 'Einstellungen', icon: '⚙️', mount: mountSettings },
};

async function boot() {
  modules = (await api('/api/core/modules')).filter(m => m.enabled);
  renderNav();
  refreshStatus();
  window.addEventListener('hashchange', route);
  route();
}

function renderNav() {
  const items = [corePages.home, ...modules, corePages.settings];
  nav.innerHTML = items.map(m => html`<a href="#/${m.id}" data-id="${m.id}"><span class="ico">${m.icon}</span><span>${m.name}</span></a>`).join('');
}

async function route() {
  const id = (location.hash.replace(/^#\//, '') || 'home').split('/')[0];
  nav.querySelectorAll('a').forEach(a => a.classList.toggle('active', a.dataset.id === id));
  view.innerHTML = '<div class="loading">Lade …</div>';
  try {
    if (corePages[id]) return await corePages[id].mount(view);
    const m = modules.find(x => x.id === id);
    if (!m) { location.hash = '#/home'; return; }
    const mod = await import(m.page + '?v=' + Date.now());
    await mod.default.mount(view, { api, toast, refreshStatus });
  } catch (e) {
    view.innerHTML = html`<div class="card error">Fehler beim Laden: ${e.message}</div>`;
  }
}

export async function refreshStatus() {
  try {
    const [h, q] = await Promise.all([api('/api/core/health'), api('/api/core/quota')]);
    const chip = (ok, label, title) => html`<span class="chip ${ok ? 'on' : 'off'}" title="${title}">${label}</span>`;
    document.getElementById('status-chips').innerHTML = [
      chip(true, 'Autocomplete', 'YouTube-Vorschläge (ohne Key)'),
      chip(true, 'Deezer', 'Künstlerdaten (ohne Key)'),
      chip(h.youtube, 'YouTube API', h.youtube ? 'Key eingetragen' : 'Kein Key - Konkurrenz nur geschätzt'),
      chip(h.lastfm, 'Last.fm', h.lastfm ? 'Key eingetragen' : 'optional'),
      chip(h.anthropic, 'Claude KI', h.anthropic ? 'Key eingetragen' : 'optional'),
      html`<div class="quota">YT heute: ${q.youtube_searches}/${q.youtube_search_budget} Suchen · ${q.youtube_units} Einh.</div>`,
    ].join('');
  } catch { /* server restarting */ }
}

// ------------------------------------------------------------------ home
async function mountHome(el) {
  const [h, q] = await Promise.all([api('/api/core/health'), api('/api/core/quota')]);
  const k = await api('/api/seo/knowledge').catch(() => null);
  const tiles = modules.map(m => html`<a class="tile" href="#/${m.id}"><div class="tile-ico">${m.icon}</div><b>${m.name}</b><small>${m.description}</small></a>`).join('');
  el.innerHTML = html`
    <h1>Willkommen 👋</h1>
    <p class="muted">Dein lokales Dashboard. Läuft nur auf deinem PC, deine Keys bleiben hier.</p>
    ${raw(!h.youtube ? `<div class="card hint">💡 <b>Tipp:</b> Trag unter <a href="#/settings">Einstellungen</a> einen kostenlosen YouTube-API-Key ein. Dann misst das Dashboard die echte Konkurrenz (Uploads, Views, Kanalgrößen) statt sie zu schätzen.</div>` : '')}
    <div class="tiles">${raw(tiles)}</div>
    <div class="grid2">
      <div class="card">
        <h3>📈 Deine Phase</h3>
        ${raw(k ? html`<p><b>${k.stage.name}</b> · ${fmt(k.subs)} Abos</p><p class="muted">${k.stage.advice}</p>` : '')}
        <p class="muted small">Abozahl unter Einstellungen pflegen oder Kanal-ID eintragen.</p>
      </div>
      <div class="card">
        <h3>🔋 YouTube-Kontingent heute</h3>
        <div class="bar"><div style="width:${Math.min(100, 100 * q.youtube_searches / q.youtube_search_budget)}%"></div></div>
        <p class="small">${q.youtube_searches} / ${q.youtube_search_budget} Suchen · ${q.youtube_units} / ${q.youtube_budget} Einheiten · Reset um 9:00 (MEZ)</p>
      </div>
    </div>
    <div class="card"><h3>🚀 Aufsteigende Künstler mit Type-Beat-Nachfrage</h3>
      <p class="muted small">Spotify-Hörer wachsen gerade (kworb.net) <i>und</i> Leute suchen nach "… type beat". Früh dran sein = wenig Konkurrenz.</p>
      <div id="rising" class="loading">Wird geladen (beim ersten Mal ~20 Sek.) …</div></div>`;
  try {
    const r = await api('/api/seo/rising');
    document.getElementById('rising').outerHTML = r.rising.length ? html`<table class="tbl"><tr><th>Künstler</th><th>Monatl. Hörer</th><th>Wachstum/Tag</th><th>Suchen</th><th></th></tr>
      ${r.rising.map(x => raw(html`<tr><td><b>${x.artist}</b></td><td>${fmt(x.listeners)}</td><td class="pos">+${x.growth_pct}%</td><td class="small muted">${x.type_beat_searches.join(' · ')}</td>
      <td><a class="btn sm" href="#/seo" data-prefill="${x.artist}">Analysieren</a></td></tr>`))}</table>`
      : '<p class="muted">Gerade keine passenden Aufsteiger gefunden.</p>';
    el.querySelectorAll('[data-prefill]').forEach(a => a.addEventListener('click', () => store('prefill', { artists: [a.dataset.prefill] })));
  } catch (e) { document.getElementById('rising').textContent = 'Konnte kworb.net nicht laden: ' + e.message; }
}

// ------------------------------------------------------------------ settings
async function mountSettings(el) {
  const [items, mods] = await Promise.all([api('/api/core/settings'), api('/api/core/modules')]);
  el.innerHTML = html`
    <h1>⚙️ Einstellungen</h1>
    <p class="muted">Gespeichert in <code>beat-dashboard/.env</code> auf deinem PC. Keys werden nie vollständig angezeigt.</p>
    <form id="sf" class="card form">
      ${items.map(s => raw(html`
        <label><span>${s.label} ${raw(s.link ? html`<a href="${s.link}" target="_blank" rel="noopener">↗</a>` : '')}</span>
          <div class="row"><input name="${s.key}" type="${s.secret ? 'password' : 'text'}" value="${s.value}" placeholder="${s.secret && s.is_set ? '(gesetzt)' : ''}" autocomplete="off">
          ${raw(['YOUTUBE_API_KEY', 'LASTFM_API_KEY', 'ANTHROPIC_API_KEY'].includes(s.key) ? html`<button type="button" class="btn sm ghost" data-test="${{ YOUTUBE_API_KEY: 'youtube', LASTFM_API_KEY: 'lastfm', ANTHROPIC_API_KEY: 'anthropic' }[s.key]}">Testen</button>` : '')}
          ${raw(s.key === 'MY_CHANNEL_ID' ? '<button type="button" class="btn sm ghost" id="sync">Abos holen</button>' : '')}</div>
          <small class="muted">${s.help || ''}</small></label>`))}
      <div class="row"><button class="btn primary">Speichern</button></div>
    </form>
    <div class="card"><h3>🧩 Module</h3><p class="muted small">Neue Module einfach als Ordner in <code>app/modules/</code> anlegen (siehe README).</p>
      ${mods.map(m => raw(html`<label class="switch"><input type="checkbox" data-mod="${m.id}" ${raw(m.enabled ? 'checked' : '')}> ${m.icon} ${m.name}</label>`))}
    </div>`;
  const form = el.querySelector('#sf');
  form.addEventListener('submit', async e => {
    e.preventDefault();
    const values = Object.fromEntries(new FormData(form));
    await api('/api/core/settings', { method: 'PUT', body: { values } });
    toast('Gespeichert ✔', 'ok'); refreshStatus(); mountSettings(el);
  });
  el.querySelectorAll('[data-test]').forEach(b => b.addEventListener('click', async () => {
    b.disabled = true; b.textContent = '…';
    const r = await api(`/api/core/settings/test/${b.dataset.test}`, { method: 'POST' });
    toast(r.message, r.ok ? 'ok' : 'err', 6000); b.disabled = false; b.textContent = 'Testen';
  }));
  el.querySelector('#sync')?.addEventListener('click', async () => {
    try { const r = await api('/api/seo/channel/sync', { method: 'POST' }); toast(`${r.subs} Abonnenten übernommen`, 'ok'); mountSettings(el); }
    catch (e) { toast(e.message, 'err'); }
  });
  el.querySelectorAll('[data-mod]').forEach(cb => cb.addEventListener('change', async () => {
    await api(`/api/core/modules/${cb.dataset.mod}`, { method: 'PUT', body: { enabled: cb.checked } });
    modules = (await api('/api/core/modules')).filter(m => m.enabled); renderNav(); toast('Module aktualisiert', 'ok');
  }));
}

boot();
