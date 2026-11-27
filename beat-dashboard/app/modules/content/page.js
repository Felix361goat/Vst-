// Content-Plan: month calendar for own posts with weekly goals.
import { api, html, raw, toast, store } from '/static/api.js';

let META = null;
const COLORS = { yt_beat: 'red', yt_short: 'orange', ig_reel: 'purple', ig_post: 'indigo', tiktok: 'teal' };
const ST = { idee: '💡', produziert: '🎞️', geplant: '🗓️', gepostet: '✓' };
const iso = d => `${d.getFullYear()}-${String(d.getMonth() + 1).padStart(2, '0')}-${String(d.getDate()).padStart(2, '0')}`;
const MONTHS = ['Jänner', 'Februar', 'März', 'April', 'Mai', 'Juni', 'Juli', 'August', 'September', 'Oktober', 'November', 'Dezember'];

export default {
  async mount(el) {
    META = await api('/api/content/meta');
    let cursor = new Date(); cursor.setDate(1);
    const pre = store('content.new');
    if (pre) { store('content.new', null); setTimeout(() => editPost(null, pre, () => draw()), 300); }
    const draw = async () => {
      const first = new Date(cursor.getFullYear(), cursor.getMonth(), 1);
      const start = new Date(first); start.setDate(1 - ((first.getDay() + 6) % 7));
      const end = new Date(start); end.setDate(start.getDate() + 41);
      const [posts, wk] = await Promise.all([api(`/api/content/posts?start=${iso(start)}&end=${iso(end)}`), api('/api/content/week')]);
      const byDay = {}; posts.forEach(p => (byDay[p.date] ||= []).push(p));
      const today = iso(new Date());
      const cells = [];
      for (let i = 0; i < 42; i++) {
        const d = new Date(start); d.setDate(start.getDate() + i);
        const k = iso(d), out = d.getMonth() !== cursor.getMonth();
        cells.push(html`<div class="cal-day ${out ? 'out' : ''} ${k === today ? 'today' : ''}" data-day="${k}">
          <div class="cal-n">${d.getDate()}</div>
          ${(byDay[k] || []).map(p => raw(html`<div class="cal-post badge ${COLORS[p.platform]} ${p.status === 'gepostet' ? 'done' : ''}" data-post="${p.id}" title="${META.platforms[p.platform]} · ${p.status}">${ST[p.status]} ${p.title || META.platforms[p.platform]}</div>`))}</div>`);
      }
      const bar = (v, g) => Math.min(100, Math.round((100 * v) / Math.max(1, g)));
      el.innerHTML = html`
        <div class="row" style="align-items:flex-end"><div><h1>Content-Plan</h1><p class="lead">Regelmäßigkeit schlägt alles – plane Beats, Shorts und Reels im Voraus.</p></div>
          <span class="spacer"></span><button class="btn primary" id="add">＋ Post planen</button></div>
        <div class="grid3">
          <div class="card stat" style="margin:0"><span class="l">Beats diese Woche</span><span class="v">${wk.beats} / ${wk.beats_goal}</span><div class="progress-line"><div style="width:${bar(wk.beats, wk.beats_goal)}%;background:var(--red)"></div></div></div>
          <div class="card stat" style="margin:0"><span class="l">Shorts / Reels / TikToks</span><span class="v">${wk.shorts} / ${wk.shorts_goal}</span><div class="progress-line"><div style="width:${bar(wk.shorts, wk.shorts_goal)}%;background:var(--purple)"></div></div></div>
          <div class="card stat" style="margin:0"><span class="l">Serie (Wochen mit Beat-Ziel)</span><span class="v">${wk.streak} 🔥</span><span class="l">${wk.open} geplante/offene Posts diese Woche</span></div>
        </div>
        ${raw(wk.today.length ? html`<div class="card hint">Heute dran: ${wk.today.map(p => `${p.time} ${META.platforms[p.platform]} – ${p.title || ''}`).join(' · ')}</div>` : '')}
        <div class="card"><div class="row"><button class="btn sm ghost" id="prev">‹</button><h2 style="margin:0;min-width:180px;text-align:center">${MONTHS[cursor.getMonth()]} ${cursor.getFullYear()}</h2><button class="btn sm ghost" id="next">›</button>
          <button class="btn sm ghost" id="today">Heute</button><span class="spacer"></span>
          ${Object.entries(META.platforms).map(([k, l]) => raw(html`<span class="badge ${COLORS[k]}">${l}</span>`))}</div>
          <div class="cal-head">${['Mo', 'Di', 'Mi', 'Do', 'Fr', 'Sa', 'So'].map(d => raw(`<div>${d}</div>`))}</div>
          <div class="cal">${raw(cells.join(''))}</div>
          <p class="small muted">Tag anklicken = neuer Post · Post anklicken = bearbeiten. 💡 Idee → 🎞️ produziert → 🗓️ geplant → ✓ gepostet.</p></div>`;
      el.querySelector('#prev').addEventListener('click', () => { cursor.setMonth(cursor.getMonth() - 1); draw(); });
      el.querySelector('#next').addEventListener('click', () => { cursor.setMonth(cursor.getMonth() + 1); draw(); });
      el.querySelector('#today').addEventListener('click', () => { cursor = new Date(); cursor.setDate(1); draw(); });
      el.querySelector('#add').addEventListener('click', () => editPost(null, { date: today }, draw));
      el.querySelectorAll('[data-day]').forEach(c => c.addEventListener('click', e => { if (!e.target.closest('[data-post]')) editPost(null, { date: c.dataset.day }, draw); }));
      el.querySelectorAll('[data-post]').forEach(b => b.addEventListener('click', () => editPost(posts.find(p => p.id == b.dataset.post), null, draw)));
    };
    draw();
  },
};

function editPost(post, defaults, onSave) {
  document.querySelector('.drawer-bg')?.remove(); document.querySelector('.drawer')?.remove();
  const p = post || { platform: 'yt_beat', status: 'idee', title: '', link: '', notes: '', time: '', ...defaults };
  const bg = document.createElement('div'); bg.className = 'drawer-bg';
  const dr = document.createElement('div'); dr.className = 'drawer'; dr.style.width = 'min(460px, 100vw)';
  document.body.append(bg, dr);
  const close = () => { bg.remove(); dr.remove(); };
  bg.addEventListener('click', close);
  dr.innerHTML = html`<div class="row"><h2 style="margin:0">${post ? 'Post bearbeiten' : 'Neuer Post'}</h2><span class="spacer"></span><button class="btn sm icon" id="x"></button></div>
    <div class="card flat form">
      <label><span>Titel</span><input data-k="title" value="${p.title || ''}" placeholder="z.B. [FREE] Ken Carson Type Beat – „Ghost“"></label>
      <div class="fields"><label><span>Plattform</span><select data-k="platform">${Object.entries(META.platforms).map(([k, l]) => raw(html`<option value="${k}" ${raw(k === p.platform ? 'selected' : '')}>${l}</option>`))}</select></label>
        <label><span>Status</span><select data-k="status">${META.statuses.map(s => raw(html`<option value="${s}" ${raw(s === p.status ? 'selected' : '')}>${ST[s]} ${s}</option>`))}</select></label>
        <label><span>Datum</span><input type="date" data-k="date" value="${p.date}"></label>
        <label><span>Uhrzeit</span><input type="time" data-k="time" value="${p.time || ''}" placeholder="${META.best_times[p.platform]}"></label></div>
      <label style="margin-top:10px"><span>Link (nach dem Posten)</span><input data-k="link" value="${p.link || ''}"></label>
      <label><span>Notizen</span><textarea data-k="notes" style="min-height:80px">${p.notes || ''}</textarea></label>
      <p class="small muted">Leer gelassene Uhrzeit = gute Standardzeit für die Plattform (Beats 18:00, Reels 19:00 …).</p>
      <div class="row"><button class="btn primary" id="save">Speichern</button>${raw(post ? '<button class="btn sm red" id="del">Löschen</button>' : '')}</div></div>`;
  dr.querySelector('#x').addEventListener('click', close);
  dr.querySelector('#save').addEventListener('click', async () => {
    const body = { ...p }; dr.querySelectorAll('[data-k]').forEach(i => { body[i.dataset.k] = i.value; });
    try {
      if (post) await api('/api/content/posts/' + post.id, { method: 'PUT', body }); else await api('/api/content/posts', { method: 'POST', body });
      toast('Gespeichert', 'ok'); close(); onSave();
    } catch (e) { toast(e.message, 'err'); }
  });
  dr.querySelector('#del')?.addEventListener('click', async () => { await api('/api/content/posts/' + post.id, { method: 'DELETE' }); close(); onSave(); });
}
