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

// drawers (slide-in panels) used by modules: Escape or navigating away closes them
document.addEventListener('keydown', e => { if (e.key === 'Escape') document.querySelector('.drawer-bg')?.click(); });
function closeDrawers() { document.querySelectorAll('.drawer, .drawer-bg').forEach(x => x.remove()); }

async function route() {
  closeDrawers();
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
    if (modules.some(m => m.id === 'leads')) {
      const st = await api('/api/leads/stats');
      const a = nav.querySelector('a[data-id=leads]');
      a?.querySelector('.badge-n')?.remove();
      if (a && st.due) a.insertAdjacentHTML('beforeend', `<span class="badge-n" title="Follow-ups fällig">${st.due}</span>`);
    }
  } catch { /* server restarting */ }
}

// ------------------------------------------------------------------ home
async function mountHome(el) {
  const [h, q] = await Promise.all([api('/api/core/health'), api('/api/core/quota')]);
  const k = await api('/api/seo/knowledge').catch(() => null);
  const st = modules.some(m => m.id === 'leads') ? await api('/api/leads/stats').catch(() => null) : null;
  const shoots = modules.some(m => m.id === 'shoots') ? await api('/api/shoots/shoots?upcoming=1').catch(() => []) : [];
  const hr = new Date().getHours();
  const hello = hr < 11 ? 'Guten Morgen' : hr < 18 ? 'Hallo' : 'Guten Abend';
  const tiles = modules.map(m => html`<a class="tile" href="#/${m.id}"><div class="tile-ico">${m.icon}</div><b>${m.name}</b><small>${m.description}</small></a>`).join('');
  el.innerHTML = html`
    <h1>${hello} 👋</h1>
    <p class="lead">${new Date().toLocaleDateString('de-AT', { weekday: 'long', day: 'numeric', month: 'long' })} · Läuft lokal auf deinem PC.</p>
    ${raw(st ? html`<div class="grid4">
      <a class="card stat tile" href="#/leads"><span class="v" style="color:${st.due ? 'var(--red)' : 'inherit'}">${st.due}</span><span class="l">Follow-ups fällig</span></a>
      <div class="card stat"><span class="v">${st.contacted_week}</span><span class="l">Kontakte diese Woche</span></div>
      <div class="card stat"><span class="v">${st.reply_rate == null ? '–' : st.reply_rate + ' %'}</span><span class="l">Antwortquote</span></div>
      <div class="card stat"><span class="v">${st.won}</span><span class="l">Gebucht · ${fmt(Math.round(st.won_value))} €</span></div></div>` : '')}
    ${raw(!h.youtube ? `<div class="card hint">💡 Trag unter <a href="#/settings">Einstellungen</a> einen kostenlosen YouTube-API-Key ein – dann misst das Dashboard echte Konkurrenz und findet Newcomer-Rapper über ihre YouTube-Kanäle.</div>` : '')}
    ${raw(shoots.length ? html`<div class="card"><h3>🎬 Nächste Drehs</h3><table class="tbl">${shoots.slice(0, 5).map(x => raw(html`<tr><td><a href="#/shoots"><b>${x.title}</b></a><div class="lead-sub">${x.location || ''}</div></td><td>${x.date}</td><td>${x.call_time || ''}</td></tr>`))}</table></div>` : '')}
    <div class="tiles">${raw(tiles)}</div>
    <div class="grid2">
      <div class="card">
        <h3>📈 Deine Phase</h3>
        ${raw(k ? html`<p><b>${k.stage.name}</b> · ${fmt(k.subs)} Abos</p><p class="muted small">${k.stage.advice}</p>` : '')}
      </div>
      <div class="card">
        <h3>🔋 YouTube-Kontingent heute</h3>
        <div class="bar"><div style="width:${Math.min(100, 100 * q.youtube_searches / q.youtube_search_budget)}%"></div></div>
        <p class="small muted">${q.youtube_searches} / ${q.youtube_search_budget} Suchen · ${q.youtube_units} / ${q.youtube_budget} Einheiten · Reset 9:00</p>
      </div>
    </div>
    <div class="card"><h3>🚀 Aufsteigende Künstler mit Type-Beat-Nachfrage</h3>
      <p class="muted small">Spotify-Hörer wachsen gerade (kworb.net) <i>und</i> Leute suchen „… type beat“. Früh dran sein = wenig Konkurrenz.</p>
      <div id="rising" class="loading"><span class="spinner"></span> Wird geladen (beim ersten Mal ~20 Sek.) …</div></div>`;
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
    <h1>Einstellungen</h1>
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
    <div class="card" id="lan"></div>
    <div class="card"><h3>🧩 Module</h3><p class="muted small">Neue Module einfach als Ordner in <code>app/modules/</code> anlegen (siehe README).</p>
      ${mods.map(m => raw(html`<label class="switch"><input type="checkbox" data-mod="${m.id}" ${raw(m.enabled ? 'checked' : '')}> ${m.icon} ${m.name}</label>`))}
    </div>`;
  api('/api/core/lan').then(l => {
    el.querySelector('#lan').innerHTML = l.enabled
      ? html`<h3>📱 Am Handy öffnen</h3><div class="row" style="gap:20px;align-items:center"><div>${raw(l.qr)}</div><div><p>Mit der Handy-Kamera scannen (gleiches WLAN):</p><p><code>${l.url}</code></p>
        <p class="small muted">Dann „Zum Home-Bildschirm“ hinzufügen – fühlt sich an wie eine App. Funktioniert nur, solange das Dashboard am PC läuft.</p></div></div>`
      : html`<h3>📱 Am Handy nutzen</h3><p class="small muted">Oben „Zugriff vom Handy“ auf <b>ja</b> setzen, eine PIN wählen, speichern und das Dashboard neu starten (start.bat). Dann erscheint hier ein QR-Code. Windows fragt beim ersten Start evtl. nach der Firewall – „Private Netzwerke“ erlauben.</p>`;
  });
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
