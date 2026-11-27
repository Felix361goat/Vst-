// YouTube SEO module page. Plain ES module: export default { mount(el, ctx) }.
import { api, html, raw, toast, copy, fmt, store } from '/static/api.js';

let K = null;            // knowledge (genres, moods …)
let last = null;         // last analysis result
let lastInput = null;

export default {
  async mount(el, ctx) {
    K = await api('/api/seo/knowledge');
    el.innerHTML = html`
      <h1>YouTube SEO</h1>
      <p class="lead">Beat beschreiben oder hochladen → das Dashboard prüft echte Suchnachfrage und Konkurrenz und schreibt Titel, Tags und Beschreibung.</p>
      <div class="tabs" id="tabs">
        <button data-t="new" class="active">🎵 Neuer Beat</button>
        <button data-t="explore">🔎 Keyword-Explorer</button>
        <button data-t="history">🗂️ Verlauf</button>
        <button data-t="uploads">📤 Uploads & Performance</button>
        <button data-t="guide">📘 Checkliste & Wissen</button>
      </div>
      <div id="tab"></div>`;
    const tabs = { new: tabNew, explore: tabExplore, history: tabHistory, uploads: tabUploads, guide: tabGuide };
    const show = t => {
      el.querySelectorAll('#tabs button').forEach(b => b.classList.toggle('active', b.dataset.t === t));
      store('seo.tab', t);
      tabs[t](el.querySelector('#tab'), ctx);
    };
    el.querySelectorAll('#tabs button').forEach(b => b.addEventListener('click', () => show(b.dataset.t)));
    show(store('prefill') ? 'new' : (store('seo.tab') || 'new'));
  },
};

// ------------------------------------------------------------------ new beat
function tabNew(el, ctx) {
  const saved = { genre: 'trap', mood: 'Dark', free: 'free', sample_source: 'original', depth: 4, ...(store('seo.form') || {}) };
  const pre = store('prefill');
  if (pre) { saved.artists = pre.artists.join(', '); store('prefill', null); }
  const genreOpts = Object.entries(K.genres).map(([id, g]) => raw(html`<option value="${id}" ${raw(id === saved.genre ? 'selected' : '')}>${g.label} (${g.bpm[0]}–${g.bpm[1]})</option>`));
  const opt = (list, cur) => list.map(([v, l]) => raw(html`<option value="${v}" ${raw(v === cur ? 'selected' : '')}>${l}</option>`));
  el.innerHTML = html`
    <div class="card">
      <div class="drop" id="drop">🎧 <b>Beat hier reinziehen</b> oder klicken (WAV, MP3, FLAC, OGG, AIFF) - BPM & Tonart werden lokal erkannt
        <input type="file" id="file" accept=".wav,.mp3,.flac,.ogg,.aif,.aiff" hidden></div>
      <div id="audio-res"></div>
    </div>
    <form class="card form" id="f">
      <div class="fields">
        <label><span>Beat-Name</span><input name="beat_name" value="${saved.beat_name || ''}" placeholder="z.B. Ghost"></label>
        <label style="grid-column: span 2"><span>Künstler (1–2, mit Komma) - leer lassen = Dashboard schlägt vor</span>
          <input name="artists" value="${saved.artists || ''}" placeholder="z.B. Ken Carson, Destroy Lonely"></label>
        <label><span>Genre</span><select name="genre">${genreOpts}</select></label>
        <label><span>Mood</span><select name="mood">${opt(K.moods.map(m => [m, m]), saved.mood)}</select></label>
        <label><span>BPM</span><input name="bpm" type="number" step="0.5" value="${saved.bpm || ''}"></label>
        <label><span>Tonart</span><input name="key" value="${saved.key || ''}" placeholder="z.B. F Minor"></label>
        <label><span>Format-Modifier</span><select name="modifier">${opt([['', '– keiner –'], ...K.modifiers.map(m => [m, m])], saved.modifier || '')}</select></label>
        <label><span>Lizenz im Titel</span><select name="free">${opt([['free', '[FREE] (nur non-profit frei)'], ['free_for_profit', '[FREE FOR PROFIT]'], ['none', 'Kein FREE-Tag']], saved.free)}</select></label>
        <label><span>Samples</span><select name="sample_source">${opt([['original', '100 % original'], ['royalty_free', 'Royalty-free Pack (Splice …)'], ['cleared', 'Sample geklärt'], ['uncleared', 'Ungeklärtes Sample ⚠️']], saved.sample_source)}</select></label>
      </div>
      <div class="row" style="margin-top:12px">
        <label class="switch"><input type="checkbox" name="use_youtube" ${raw(saved.use_youtube === false ? '' : 'checked')}> YouTube-Konkurrenz live prüfen</label>
        <label class="switch">Tiefe <input name="youtube_depth" type="number" min="0" max="8" value="${saved.depth}" style="width:60px"> Keywords <span class="muted small">(je 2 von ~100 Tages-Suchen)</span></label>
        <label class="switch"><input type="checkbox" name="use_llm" ${raw(saved.use_llm ? 'checked' : '')}> Claude-KI-Texte</label>
        <span style="flex:1"></span>
        <button class="btn primary" id="go">🔍 Analysieren</button>
      </div>
    </form>
    <div id="res"></div>`;

  const form = el.querySelector('#f');
  const drop = el.querySelector('#drop'), file = el.querySelector('#file');
  let audioFile = null;
  drop.addEventListener('click', () => file.click());
  drop.addEventListener('dragover', e => { e.preventDefault(); drop.classList.add('over'); });
  drop.addEventListener('dragleave', () => drop.classList.remove('over'));
  drop.addEventListener('drop', e => { e.preventDefault(); drop.classList.remove('over'); if (e.dataTransfer.files[0]) upload(e.dataTransfer.files[0]); });
  file.addEventListener('change', () => file.files[0] && upload(file.files[0]));

  async function upload(f) {
    const out = el.querySelector('#audio-res');
    out.innerHTML = html`<p class="muted"><span class="spinner"></span> Analysiere ${f.name} …</p>`;
    const fd = new FormData(); fd.append('file', f);
    try {
      const r = await api('/api/seo/audio/analyze', { method: 'POST', form: fd });
      audioFile = r.file;
      form.bpm.value = r.bpm; form.key.value = r.key;
      if (!form.beat_name.value) form.beat_name.value = r.name_suggestion.replace(/[_-]+/g, ' ').replace(/\b(\d{2,3} ?bpm|beat|final|master|v\d)\b/gi, '').trim();
      if (r.genre_matches.length && !r.genre_matches.includes(form.genre.value)) form.genre.value = r.genre_matches[0];
      out.innerHTML = html`<div class="audio-res">
        <div><div class="muted small">BPM</div><div class="big">${r.bpm}</div>
          <div class="row">${r.bpm_alternatives.map(b => raw(html`<button type="button" class="btn sm ghost" data-bpm="${b}">${b}?</button>`))}</div></div>
        <div><div class="muted small">Tonart</div><div class="big">${r.key}</div><div class="small muted">Camelot ${r.camelot} · Paralleltonart ${r.relative} · Alternative: <a href="#" data-key="${r.alternative}">${r.alternative}</a></div></div>
        <div><div class="muted small">Sicherheit</div><div class="big">${Math.round(r.confidence * 100)}%</div><div class="small muted">Engine: ${r.engine}</div></div>
        <div><div class="muted small">Passende Genres</div><div>${r.genre_matches.map(g => K.genres[g]?.label).join(', ') || '–'}</div></div></div>
        <p class="small muted">Trap/Drill wird oft halb oder doppelt gezählt - klick auf die Alternative, falls du anders zählst. Tonart vor dem Upload kurz gegenchecken.</p>`;
      out.querySelectorAll('[data-bpm]').forEach(b => b.addEventListener('click', () => { form.bpm.value = b.dataset.bpm; toast(`BPM = ${b.dataset.bpm}`, 'ok', 1200); }));
      out.querySelector('[data-key]').addEventListener('click', e => { e.preventDefault(); form.key.value = e.target.dataset.key; });
    } catch (e) { out.innerHTML = html`<p class="neg">${e.message}</p>`; }
  }

  form.addEventListener('submit', async e => {
    e.preventDefault();
    const fd = Object.fromEntries(new FormData(form));
    store('seo.form', { ...fd, use_youtube: !!fd.use_youtube, use_llm: !!fd.use_llm, depth: fd.youtube_depth });
    const body = {
      ...fd, artists: fd.artists.split(/[,;]| x /i).map(s => s.trim()).filter(Boolean),
      bpm: fd.bpm ? Number(fd.bpm) : null, key: fd.key || null, use_youtube: !!fd.use_youtube,
      use_llm: !!fd.use_llm, youtube_depth: Number(fd.youtube_depth || 0), audio_file: audioFile,
    };
    const btn = el.querySelector('#go'); btn.disabled = true;
    const res = el.querySelector('#res');
    res.innerHTML = '<div class="card"><span class="spinner"></span> Recherchiere: Autocomplete, ähnliche Künstler, Aufsteiger' + (body.use_youtube ? ', YouTube-Konkurrenz' : '') + (body.use_llm ? ', Claude' : '') + ' … (beim ersten Mal 20–60 Sek., danach aus dem Cache)</div>';
    try {
      last = await api('/api/seo/analyze', { method: 'POST', body });
      lastInput = body;
      renderResult(res, last, body, ctx);
      ctx.refreshStatus();
    } catch (err) { res.innerHTML = html`<div class="card error">${err.message}</div>`; }
    btn.disabled = false;
  });
}

const scoreColor = s => (s >= 65 ? 'var(--gold)' : s >= 45 ? 'var(--ok)' : 'var(--err)');

export function renderResult(res, r, input, ctx) {
  const rows = r.candidates.map((c, i) => {
    const n = c.niche, s = c.score;
    return raw(html`<tr>
      <td><b>${c.phrase}</b><div class="small muted">${c.why}${raw(c.rising ? ' · <span class="pos">🚀 steigt</span>' : '')}</div></td>
      <td><span class="score"><span class="b"><i style="width:${s.opportunity}%;background:${scoreColor(s.opportunity)}"></i></span><b>${s.opportunity}</b></span>
        <span class="badge ${s.label}">${s.label}</span>${raw(s.estimated ? ' <span class="badge est" title="Ohne YouTube-Daten geschätzt">geschätzt</span>' : '')}</td>
      <td>${Math.round(s.demand * 100)}</td><td>${Math.round(s.competition * 100)}</td>
      <td>${n ? (n.uploads_30d_capped ? '50+' : n.uploads_30d) : '–'}</td>
      <td>${n ? fmt(n.median_views) : '–'}</td><td>${n ? fmt(n.median_subs_top10) : '–'}</td>
      <td>${n ? n.outliers.length : '–'}</td>
      <td>${fmt(c.popularity.value)}<div class="small muted">${c.popularity.unit || ''}</div></td>
      <td>${raw(c.artist ? html`<button class="btn sm" data-use="${i}">Verwenden</button>` : '')}</td></tr>`);
  });
  const best = r.candidates.find(c => c.niche) || null;
  res.innerHTML = html`
    ${raw(r.recommendation ? html`<div class="card reco">💡 ${r.recommendation}</div>` : '')}
    ${raw(r.notes.map(n => html`<div class="card hint small">ℹ️ ${n}</div>`).join(''))}
    <div class="card"><h3>📊 Keyword-Chancen</h3>
      <p class="small muted">Chance = Nachfrage (Autocomplete, Popularität, Views) gegen Konkurrenz (Uploads der letzten 30 Tage, Kanalgrößen der Top 10). Phase: <b>${r.stage.name}</b>. ${r.stage.advice}</p>
      <div style="overflow-x:auto"><table class="tbl"><tr><th>Keyword</th><th>Chance</th><th>Nachfrage</th><th>Konkurrenz</th><th title="Uploads mit diesem Keyword in den letzten 30 Tagen">Uploads 30T</th><th title="Median-Views der Top-Ergebnisse">Ø Views</th><th title="Median-Abos der Top-10-Kanäle">Ø Abos Top10</th><th title="Videos mit überdurchschnittlich vielen Views (frisch)">Outlier</th><th>Popularität</th><th></th></tr>${rows}</table></div>
    </div>
    <div id="gen"></div>
    ${raw(r.ai ? renderAI(r.ai) : '')}
    <div class="grid2">
      <div class="card"><h3>⏰ Wann posten die Top-Videos?</h3>${raw(renderHours(r.publish_hours_utc, r.publish_weekdays))}</div>
      <div class="card"><h3>🏷️ Tags der Konkurrenz</h3>${raw(r.competitor_tags.length ? html`<div class="tags">${r.competitor_tags.map(([t, n]) => raw(html`<span class="tag" title="${n}× verwendet">${t} <span class="muted">${n}</span></span>`))}</div>` : '<p class="muted small">Braucht YouTube-API-Key.</p>')}</div>
    </div>
    ${raw(best ? renderVideos(best) : '')}
    <div class="card"><h3>🔗 Ähnliche Künstler (für "x"-Kombos)</h3>${raw(r.related.length ? html`<div class="tags">${r.related.map(a => raw(html`<span class="tag" title="via ${a.via.join(', ')} · ${a.sources.join('+')}">${a.name}</span>`))}</div>` : '<p class="muted small">Keine Künstler angegeben.</p>')}</div>`;
  renderGenerated(res.querySelector('#gen'), r.generated, r, input);
  res.querySelectorAll('[data-use]').forEach(b => b.addEventListener('click', async () => {
    const c = r.candidates[Number(b.dataset.use)];
    const artists = c.phrase.includes(' x ') ? (input.artists || []).slice(0, 2) : [c.artist];
    const g = await api('/api/seo/generate', { method: 'POST', body: { ...input, artists, keywords: r.keywords } });
    r.artists_used = artists;
    renderGenerated(res.querySelector('#gen'), g, r, { ...input, artists });
    res.querySelector('#gen').scrollIntoView({ behavior: 'smooth' });
    toast(`Texte für "${c.phrase}" erstellt`, 'ok');
  }));
}

function renderGenerated(el, g, r, input) {
  el.innerHTML = html`
    <div class="card"><h3>✍️ Titel <span class="muted small">(klicken = kopieren)</span></h3>
      <div class="titles">${g.titles.map(t => raw(html`<div class="t" data-copy="${t.text}"><span>${t.text}</span><span class="len">${t.length} Z.${raw(t.keyword_end ? html` · Keyword bis ${t.keyword_end}` : '')}</span></div>`))}</div>
      <label class="form" style="display:block;margin-top:10px"><span class="small muted">Eigener Titel - wird live geprüft:</span>
        <input id="own-title" value="${g.titles[0]?.text || ''}"></label>
      <ul class="checks" id="checks">${g.checks.map(c => raw(html`<li class="lvl-${c.level}">${c.text}</li>`))}</ul>
    </div>
    <div class="grid2">
      <div class="card"><h3>🏷️ Tags <span class="muted small">${g.tags.length} Tags · ${g.tag_chars}/500 Zeichen</span></h3>
        <div class="tags">${g.tags.map(t => raw(html`<span class="tag">${t}</span>`))}</div>
        <div class="row" style="margin-top:10px"><button class="btn sm" data-copy="${g.tags_csv}">Alle Tags kopieren</button></div>
        <h3 style="margin-top:16px">#️⃣ Hashtags</h3><div class="row"><code>${g.hashtags.join(' ')}</code><button class="btn sm ghost" data-copy="${g.hashtags.join(' ')}">Kopieren</button></div>
        <p class="small muted">Stehen schon am Ende der Beschreibung - die ersten 3 erscheinen über dem Titel.</p>
      </div>
      <div class="card"><h3>📌 Angepinnter Kommentar</h3><textarea style="min-height:70px" id="pin">${g.pinned_comment}</textarea>
        <button class="btn sm" data-copy-from="pin">Kopieren</button>
        <h3 style="margin-top:16px">📱 Short-Titel</h3><div class="row"><code>${g.short_title}</code><button class="btn sm ghost" data-copy="${g.short_title}">Kopieren</button></div>
        <h3 style="margin-top:16px">📂 Playlists</h3><div class="tags">${g.playlists.map(p => raw(html`<span class="tag">${p}</span>`))}</div></div>
    </div>
    <div class="card"><h3>📝 Beschreibung</h3><textarea id="desc" style="min-height:280px">${g.description}</textarea>
      <div class="row"><button class="btn sm" data-copy-from="desc">Beschreibung kopieren</button>
        <button class="btn sm ghost" id="copy-all">Alles kopieren (Titel + Beschreibung + Tags)</button>
        <button class="btn sm ghost" id="save-upload">📤 Als Upload merken</button>
        <span class="small muted">Store-Link, Name & Kontakt unter Einstellungen hinterlegen.</span></div></div>`;
  el.querySelectorAll('[data-copy]').forEach(b => b.addEventListener('click', () => copy(b.dataset.copy)));
  el.querySelectorAll('[data-copy-from]').forEach(b => b.addEventListener('click', () => copy(el.querySelector('#' + b.dataset.copyFrom).value)));
  const own = el.querySelector('#own-title');
  el.querySelectorAll('.titles .t').forEach(t => t.addEventListener('click', () => { own.value = t.dataset.copy; recheck(); }));
  let timer;
  const recheck = async () => {
    const c = await api('/api/seo/check', { method: 'POST', body: { title: own.value, description: el.querySelector('#desc').value, tags: g.tags, artists: r.artists_used || input.artists || [], genre: input.genre, bpm: input.bpm, free: input.free, sample_source: input.sample_source } });
    el.querySelector('#checks').innerHTML = c.checks.map(x => html`<li class="lvl-${x.level}">${x.text}</li>`).join('');
  };
  own.addEventListener('input', () => { clearTimeout(timer); timer = setTimeout(recheck, 350); });
  el.querySelector('#desc').addEventListener('input', () => { clearTimeout(timer); timer = setTimeout(recheck, 600); });
  el.querySelector('#copy-all').addEventListener('click', () => copy(`${own.value}\n\n${el.querySelector('#desc').value}\n\nTAGS:\n${g.tags_csv}`, 'Alles kopiert'));
  el.querySelector('#save-upload').addEventListener('click', async () => {
    const url = prompt('YouTube-Link oder Video-ID (kannst du auch später unter "Uploads" eintragen):', '');
    if (url === null) return;
    const kw = (r.artists_used?.[0] ? r.artists_used[0].toLowerCase() + ' type beat' : '');
    await api('/api/seo/uploads', { method: 'POST', body: { analysis_id: r.id, youtube_url: url, title: own.value, keyword: kw, tags: g.tags_csv } });
    toast('Upload gespeichert - Performance unter "Uploads & Performance"', 'ok');
  });
}

function renderAI(ai) {
  if (ai.error) return html`<div class="card hint">🤖 ${ai.error}</div>`;
  return html`<div class="card"><h3>🤖 Claude-Vorschläge</h3>
    ${raw(ai.tip ? html`<p>💡 ${ai.tip}</p>` : '')}
    <div class="titles">${(ai.titles || []).map(t => raw(html`<div class="t" onclick="navigator.clipboard.writeText(this.dataset.c)" data-c="${t}"><span>${t}</span><span class="len">${t.length} Z.</span></div>`))}</div>
    ${raw(ai.thumbnail_text ? html`<p><b>Thumbnail-Text:</b> ${ai.thumbnail_text.join(' · ')}</p>` : '')}
    ${raw(ai.extra_tags?.length ? html`<p><b>Extra-Tags:</b> ${ai.extra_tags.join(', ')}</p>` : '')}
    ${raw(ai.description ? html`<details><summary>KI-Beschreibung anzeigen</summary><textarea style="min-height:220px">${ai.description}</textarea></details>` : '')}</div>`;
}

function renderHours(hoursUtc, weekdays) {
  const total = hoursUtc.reduce((a, b) => a + b, 0);
  const off = -new Date().getTimezoneOffset() / 60;
  const local = Array.from({ length: 24 }, (_, h) => hoursUtc[((h - off) % 24 + 24) % 24]);
  if (!total) {
    return html`<p class="muted small">Ohne YouTube-Daten: Faustregel - 2–4 Std. vor dem Abend-Peak in den USA posten, also ca. <b>18:00–23:00 Uhr deutscher Zeit</b>, Di–Fr. Nach 30 Tagen in YouTube Studio unter "Wann deine Zuschauer auf YouTube sind" nachschauen.</p>`;
  }
  const max = Math.max(...local);
  const bestH = local.indexOf(max);
  const days = ['Mo', 'Di', 'Mi', 'Do', 'Fr', 'Sa', 'So'];
  const bestD = days[weekdays.indexOf(Math.max(...weekdays))];
  return html`<div class="hours">${local.map((v, h) => raw(html`<div style="height:${(100 * v) / max}%" title="${h}:00 – ${v} Videos"></div>`))}</div>
    <div class="hours-l"><span>0 Uhr</span><span>6</span><span>12</span><span>18</span><span>23</span></div>
    <p class="small">Häufigste Upload-Stunde der Top-Videos: <b>${bestH}:00 Uhr</b> (deine Zeit) · häufigster Tag: <b>${bestD}</b>. Tipp: 1–2 Std. <i>davor</i> hochladen, damit dein Video schon indexiert ist.</p>`;
}

function renderVideos(c) {
  const vids = (c.niche.outliers.length ? c.niche.outliers : c.niche.top_videos).slice(0, 8);
  return html`<div class="card"><h3>${c.niche.outliers.length ? '🔥 Outlier' : '🏆 Top-Videos'} für "${c.phrase}"</h3>
    <p class="small muted">${c.niche.outliers.length ? 'Videos mit deutlich mehr Views als ihr Kanal Abos hat - hier funktioniert gerade etwas. Thumbnails & Titel anschauen!' : 'Was aktuell oben rankt.'}</p>
    <div class="vids">${vids.map(v => raw(html`<a class="vid" href="https://youtu.be/${v.id}" target="_blank" rel="noopener"><img src="${v.thumb || ''}" loading="lazy" alt=""><div><b>${v.title}</b><br><span class="muted">${v.channel} · ${fmt(v.channel_subs)} Abos<br>${fmt(v.views)} Views · vor ${Math.round(v.age_days)} Tagen</span></div></a>`))}</div></div>`;
}

// ------------------------------------------------------------------ keyword explorer
function tabExplore(el, ctx) {
  el.innerHTML = html`
    <div class="card form"><div class="row"><input id="q" placeholder="z.B. ken carson type beat  oder  dark plugg" value="${store('seo.q') || ''}">
      <button class="btn" id="s">Vorschläge</button><button class="btn" id="lt">Long-Tail (a–z)</button><button class="btn" id="yt">YouTube-Check (2 Suchen)</button></div>
      <p class="small muted">Vorschläge & Long-Tail sind kostenlos (YouTube-Autocomplete). Der YouTube-Check verbraucht 2 der ~100 täglichen Suchen (danach 2 Tage im Cache).</p></div>
    <div id="out"></div>`;
  const q = el.querySelector('#q'), out = el.querySelector('#out');
  const run = async (fn) => { store('seo.q', q.value); out.innerHTML = '<p class="muted"><span class="spinner"></span> Lade …</p>'; try { await fn(q.value.trim()); } catch (e) { out.innerHTML = html`<div class="card error">${e.message}</div>`; } };
  el.querySelector('#s').addEventListener('click', () => run(async v => {
    const r = await api('/api/seo/suggest', { method: 'POST', body: { q: v } });
    out.innerHTML = html`<div class="card"><h3>Autocomplete für "${v}"</h3>
      <p class="small">Nachfrage-Signal: <b>${Math.round(r.probe.score * 100)}/100</b> ${raw(r.probe.found_at ? html`<span class="muted">(erscheint schon beim Tippen von "${r.probe.found_at.prefix}" auf Platz ${r.probe.found_at.rank})</span>` : '<span class="muted">(taucht beim Tippen des Namens nicht auf)</span>')}</p>
      <div class="tags">${r.suggestions.map(s => raw(html`<span class="tag">${s}</span>`))}</div>
      <h3 style="margin-top:14px">Erweiterungen</h3><div class="tags">${r.probe.expansions.map(s => raw(html`<span class="tag">${s}</span>`))}</div></div>`;
  }));
  el.querySelector('#lt').addEventListener('click', () => run(async v => {
    const r = await api('/api/seo/longtail', { method: 'POST', body: { q: v } });
    out.innerHTML = html`<div class="card"><h3>${r.keywords.length} Long-Tail-Keywords</h3><div class="tags">${r.keywords.map(s => raw(html`<span class="tag">${s}</span>`))}</div>
      <button class="btn sm" style="margin-top:10px" id="cp">Alle kopieren</button></div>`;
    out.querySelector('#cp').addEventListener('click', () => copy(r.keywords.join(', ')));
  }));
  el.querySelector('#yt').addEventListener('click', () => run(async v => {
    const n = await api('/api/seo/niche', { method: 'POST', body: { q: v } });
    ctx.refreshStatus();
    out.innerHTML = html`<div class="card"><h3>YouTube-Nische "${n.phrase}" ${raw(n.cached ? '<span class="badge">aus Cache</span>' : '')}</h3>
      <div class="grid3">
        <div><div class="muted small">Uploads letzte 30 Tage</div><div style="font-size:22px;font-weight:700">${n.uploads_30d_capped ? '50+' : n.uploads_30d}</div><div class="small muted">&lt;50 offen · 50–300 machbar · 1000+ gesättigt</div></div>
        <div><div class="muted small">Ø Views Top-Videos</div><div style="font-size:22px;font-weight:700">${fmt(n.median_views)}</div></div>
        <div><div class="muted small">Ø Abos Top-10-Kanäle</div><div style="font-size:22px;font-weight:700">${fmt(n.median_subs_top10)}</div><div class="small muted">${n.small_channels_top10} Kanäle unter 10k in Top 10</div></div>
      </div>
      <p class="small">Frische Videos (&lt;30 Tage) in den Top-Ergebnissen: <b>${Math.round(n.fresh_share * 100)}%</b> · exaktes Keyword im Titel: ${Math.round(n.exact_title_share * 100)}%</p>
      <h3>Häufigste Tags</h3><div class="tags">${n.top_tags.map(([t, c]) => raw(html`<span class="tag">${t} <span class="muted">${c}</span></span>`))}</div></div>
      ${raw(renderVideos({ phrase: n.phrase, niche: n }))}`;
  }));
  q.addEventListener('keydown', e => { if (e.key === 'Enter') el.querySelector('#s').click(); });
}

// ------------------------------------------------------------------ history
async function tabHistory(el, ctx) {
  const list = await api('/api/seo/analyses');
  if (!list.length) { el.innerHTML = '<div class="card muted">Noch keine Analysen.</div>'; return; }
  el.innerHTML = html`<div class="card"><table class="tbl"><tr><th>Datum</th><th>Beat</th><th>Künstler</th><th>Genre</th><th>BPM</th><th>Key</th><th></th></tr>
    ${list.map(a => raw(html`<tr><td class="small">${a.created_at}</td><td><b>${a.beat_name || '–'}</b></td><td>${(a.artists || []).join(' x ')}</td><td>${K.genres[a.genre]?.label || a.genre}</td><td>${a.bpm || '–'}</td><td>${a.musical_key || '–'}</td>
      <td class="row"><button class="btn sm" data-open="${a.id}">Öffnen</button><button class="btn sm ghost" data-del="${a.id}">🗑</button></td></tr>`))}</table></div><div id="detail"></div>`;
  el.querySelectorAll('[data-open]').forEach(b => b.addEventListener('click', async () => {
    const a = await api('/api/seo/analyses/' + b.dataset.open);
    const d = el.querySelector('#detail');
    renderResult(d, { ...a.result, id: a.id }, a.input, ctx);
    d.scrollIntoView({ behavior: 'smooth' });
  }));
  el.querySelectorAll('[data-del]').forEach(b => b.addEventListener('click', async () => {
    if (!confirm('Analyse löschen?')) return;
    await api('/api/seo/analyses/' + b.dataset.del, { method: 'DELETE' }); tabHistory(el, ctx);
  }));
}

// ------------------------------------------------------------------ uploads
async function tabUploads(el, ctx) {
  const list = await api('/api/seo/uploads');
  el.innerHTML = html`
    <div class="card form"><h3>Upload hinzufügen</h3><div class="row">
      <input id="u-url" placeholder="YouTube-Link"><input id="u-kw" placeholder="Haupt-Keyword (z.B. ken carson type beat)"><button class="btn" id="u-add">Hinzufügen</button>
      <button class="btn primary" id="u-ref">🔄 Views aktualisieren</button></div>
      <p class="small muted">Aktualisieren kostet nur 1 YouTube-Einheit pro 50 Videos. Check-Punkte: 48 h (CTR in Studio, Ziel 4–8 %), 7 Tage (≥ 2× deines Kanal-Medians, sonst Keyword wechseln), 30 Tage (Verkäufe).</p></div>
    <div class="card">${raw(list.length ? html`<table class="tbl"><tr><th>Video</th><th>Keyword</th><th>Veröffentlicht</th><th>Views</th><th>Likes</th><th>Komm.</th><th>Stand</th><th></th></tr>
      ${list.map(u => raw(html`<tr><td>${raw(u.youtube_video_id ? html`<a href="https://youtu.be/${u.youtube_video_id}" target="_blank" rel="noopener">${u.title || u.youtube_video_id}</a>` : html`${u.title || '–'}`)}</td>
      <td>${u.keyword || '–'}</td><td class="small">${(u.published_at || '').slice(0, 10) || '–'}</td><td><b>${fmt(u.views)}</b></td><td>${fmt(u.likes)}</td><td>${fmt(u.comments)}</td><td class="small muted">${u.fetched_at || '–'}</td>
      <td><button class="btn sm ghost" data-del="${u.id}">🗑</button></td></tr>`))}</table>${raw(renderKeywordPerf(list))}` : '<p class="muted">Noch keine Uploads gespeichert. Nach einer Analyse "📤 Als Upload merken" klicken oder hier einen Link eintragen.</p>')}</div>`;
  el.querySelector('#u-add').addEventListener('click', async () => {
    await api('/api/seo/uploads', { method: 'POST', body: { youtube_url: el.querySelector('#u-url').value, keyword: el.querySelector('#u-kw').value } });
    tabUploads(el, ctx);
  });
  el.querySelector('#u-ref').addEventListener('click', async () => {
    try { const r = await api('/api/seo/uploads/refresh', { method: 'POST' }); toast(`${r.updated} Videos aktualisiert`, 'ok'); ctx.refreshStatus(); tabUploads(el, ctx); }
    catch (e) { toast(e.message, 'err'); }
  });
  el.querySelectorAll('[data-del]').forEach(b => b.addEventListener('click', async () => { await api('/api/seo/uploads/' + b.dataset.del, { method: 'DELETE' }); tabUploads(el, ctx); }));
}

function renderKeywordPerf(list) {
  const by = {};
  list.filter(u => u.views != null && u.keyword).forEach(u => { (by[u.keyword] ||= []).push(u.views); });
  const rows = Object.entries(by).map(([k, v]) => [k, v.length, Math.round(v.reduce((a, b) => a + b, 0) / v.length)]).sort((a, b) => b[2] - a[2]);
  if (!rows.length) return '';
  return html`<h3 style="margin-top:18px">Welche Keywords funktionieren für dich?</h3><table class="tbl"><tr><th>Keyword</th><th>Uploads</th><th>Ø Views</th></tr>
    ${rows.map(([k, n, v]) => raw(html`<tr><td>${k}</td><td>${n}</td><td><b>${fmt(v)}</b></td></tr>`))}</table>`;
}

// ------------------------------------------------------------------ guide
function tabGuide(el) {
  el.innerHTML = html`
    <div class="card"><h3>✅ Upload-Checkliste</h3><ul class="checks">${K.checklist.map((c, i) => raw(html`<li><label class="switch"><input type="checkbox" data-i="${i}"> ${c}</label></li>`))}</ul>
      <button class="btn sm ghost" id="reset">Haken zurücksetzen</button></div>
    <div class="card"><h3>🎼 Genres, BPM & typische Künstler</h3><div style="overflow-x:auto"><table class="tbl"><tr><th>Genre</th><th>BPM</th><th>Keywords</th><th>Künstler (Startliste)</th></tr>
      ${Object.values(K.genres).map(g => raw(html`<tr><td><b>${g.label}</b></td><td>${g.bpm[0]}–${g.bpm[1]}</td><td class="small">${g.keywords.join(', ')}</td><td class="small">${g.artists.join(', ')}</td></tr>`))}</table></div>
      <p class="small muted">Liste anpassen: <code>app/modules/seo/services/knowledge.py</code>. Das Dashboard prüft die Nachfrage sowieso live.</p></div>
    <div class="grid2">
      <div class="card"><h3>📏 Titel-Regeln</h3><ul class="small">
        <li>"Künstler Type Beat" in den ersten ~40 Zeichen, gesamt 55–80 Zeichen</li><li>Max. 2 Künstler mit "x", ähnlichster zuerst</li>
        <li>Beat-Name in Anführungszeichen, 1–2 Wörter</li><li>[FREE] nur, wenn non-profit Nutzung wirklich frei ist</li>
        <li>Jahr (${K.year}) bei Trend-Genres, ab 1. Nov. schon das nächste Jahr</li><li>Nie: ft., feat, leak, official, unreleased, echte Songtitel</li></ul></div>
      <div class="card"><h3>📅 Kanal-Routine</h3><ul class="small">
        <li>Mind. 3 Uploads/Woche, 2–3 Wochen Vorrat einplanen</li><li>1–2 Shorts pro Beat (15–30 Sek. Loop)</li>
        <li>Jedes Keyword alle 7 Tage neu prüfen - Trends drehen sich alle 30–60 Tage</li>
        <li>Bei Album-/Single-Release eines Künstlers: innerhalb 0–48 h hochladen</li><li>Unter 2× Kanal-Median nach 7 Tagen → anderes Keyword</li></ul></div>
    </div>`;
  const done = new Set(store('seo.check') || []);
  el.querySelectorAll('[data-i]').forEach(cb => {
    cb.checked = done.has(cb.dataset.i);
    cb.addEventListener('change', () => { cb.checked ? done.add(cb.dataset.i) : done.delete(cb.dataset.i); store('seo.check', [...done]); });
  });
  el.querySelector('#reset').addEventListener('click', () => { store('seo.check', []); tabGuide(el); });
}
