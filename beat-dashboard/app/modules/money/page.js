// Angebote, Rechnungen, Einnahmen & Ausgaben.
import { api, html, raw, toast, fmt, store } from '/static/api.js';

let META = null;
const eur = v => (Number(v) || 0).toLocaleString('de-AT', { style: 'currency', currency: 'EUR' });
const STATUS = {
  entwurf: ['Entwurf', 'gray'], gesendet: ['Gesendet', 'blue'], angenommen: ['Angenommen', 'green'], abgelehnt: ['Abgelehnt', 'red'],
  bezahlt: ['Bezahlt', 'green'], storniert: ['Storniert', 'red'],
};
const st = s => { const [l, c] = STATUS[s] || [s, 'gray']; return html`<span class="badge ${c}">${l}</span>`; };

export default {
  async mount(el) {
    META = await api('/api/money/meta');
    el.innerHTML = html`
      <h1>Angebote & Rechnungen</h1>
      <p class="lead">Vom Angebot zur bezahlten Rechnung – mit Zahlungs-QR-Code und Überblick über deine Einnahmen.</p>
      ${raw(!META.biz.BIZ_NAME || !META.biz.IBAN ? '<div class="card hint">💡 Trag unter <a href="#/settings">Einstellungen</a> Namen, Anschrift und IBAN ein – das sind Pflichtangaben auf Rechnungen (und für den QR-Code).</div>' : '')}
      <div class="tabs" id="tabs"><button data-t="overview">Übersicht</button><button data-t="docs">Dokumente</button><button data-t="ledger">Einnahmen & Ausgaben</button></div>
      <div id="tab"></div>`;
    const tabs = { overview: tabOverview, docs: tabDocs, ledger: tabLedger };
    const show = t => { el.querySelectorAll('#tabs button').forEach(b => b.classList.toggle('active', b.dataset.t === t)); store('money.tab', t); tabs[t](el.querySelector('#tab')); };
    el.querySelectorAll('#tabs button').forEach(b => b.addEventListener('click', () => show(b.dataset.t)));
    const pending = store('money.newFor');
    if (pending) {
      store('money.newFor', null);
      show('docs');
      const doc = await api('/api/money/documents', { method: 'POST', body: { type: 'angebot', lead_id: pending } });
      openDoc(doc.id, () => show('docs'));
    } else show(store('money.tab') || 'overview');
  },
};

async function tabOverview(el) {
  const o = await api('/api/money/overview');
  const max = Math.max(1, ...Object.values(o.months).flatMap(m => [m.in, m.out]));
  const mn = ['J', 'F', 'M', 'A', 'M', 'J', 'J', 'A', 'S', 'O', 'N', 'D'];
  el.innerHTML = html`
    <div class="grid4" style="margin-top:14px">
      <div class="card stat"><span class="v">${eur(o.income)}</span><span class="l">Einnahmen ${o.year}</span></div>
      <div class="card stat"><span class="v">${eur(o.expenses)}</span><span class="l">Ausgaben</span></div>
      <div class="card stat"><span class="v" style="color:${o.profit >= 0 ? 'inherit' : 'var(--red)'}">${eur(o.profit)}</span><span class="l">Gewinn (vor Steuer)</span></div>
      <div class="card stat"><span class="v">${eur(o.open_invoices.reduce((a, d) => a + d.total, 0))}</span><span class="l">${o.open_invoices.length} offene Rechnungen${o.overdue.length ? ` · ${o.overdue.length} überfällig` : ''}</span></div>
    </div>
    ${raw(o.ku ? html`<div class="card"><div class="row"><h3 style="margin:0">Kleinunternehmergrenze</h3><span class="spacer"></span><span class="small muted">${eur(o.income)} von ${eur(o.ku_limit)} (${o.ku_pct} %)</span></div>
      <div class="progress-line" style="height:8px"><div style="width:${Math.min(100, o.ku_pct)}%;background:${o.ku_pct > 90 ? 'var(--red)' : o.ku_pct > 75 ? 'var(--orange)' : 'var(--green)'}"></div></div>
      <p class="small muted">Bis 55.000 € Umsatz (brutto) pro Jahr keine Umsatzsteuer. Wird die Grenze um max. 10 % überschritten, gilt die Befreiung noch bis Jahresende. ${o.ku_pct > 75 ? '⚠️ Rechtzeitig mit Steuerberater:in sprechen.' : ''}</p></div>` : '')}
    <div class="grid2">
      <div class="card"><h3>Monate ${o.year}</h3>
        <div style="display:flex;align-items:flex-end;gap:6px;height:140px">${Object.entries(o.months).map(([m, v], i) => raw(html`<div style="flex:1;display:flex;flex-direction:column;align-items:center;gap:3px;height:100%;justify-content:flex-end" title="${eur(v.in)} / ${eur(v.out)}">
          <div style="display:flex;gap:2px;align-items:flex-end;height:100%;width:100%;justify-content:center"><div style="width:42%;background:var(--green);border-radius:3px 3px 0 0;height:${(100 * v.in) / max}%"></div><div style="width:42%;background:var(--quaternary);border-radius:3px 3px 0 0;height:${(100 * v.out) / max}%"></div></div>
          <span class="small muted">${mn[i]}</span></div>`))}</div>
        <p class="small muted"><span class="dot" style="background:var(--green)"></span> Einnahmen &nbsp; <span class="dot" style="background:var(--quaternary)"></span> Ausgaben</p></div>
      <div class="card"><h3>Offene Rechnungen</h3>${raw(o.open_invoices.length ? html`<table class="tbl">${o.open_invoices.map(d => raw(html`<tr><td><b>${d.number}</b><div class="lead-sub">${d.client_name}</div></td><td>${eur(d.total)}</td>
        <td class="small ${d.due_date < new Date().toISOString().slice(0, 10) ? 'neg' : 'muted'}">fällig ${d.due_date}</td><td><button class="btn sm green" data-paid="${d.id}">Bezahlt</button></td></tr>`))}</table>` : '<p class="small muted">Alles bezahlt. 🎉</p>')}
        <p class="small muted">${o.open_quotes.count} offene Angebote über ${eur(o.open_quotes.total)}</p></div>
    </div>`;
  el.querySelectorAll('[data-paid]').forEach(b => b.addEventListener('click', async () => {
    await api(`/api/money/documents/${b.dataset.paid}/status`, { method: 'POST', body: { status: 'bezahlt' } }); toast('Als bezahlt verbucht', 'ok'); tabOverview(el);
  }));
}

async function tabDocs(el) {
  const docs = await api('/api/money/documents');
  el.innerHTML = html`
    <div class="row" style="margin:14px 0"><button class="btn primary" id="new-a">＋ Angebot</button><button class="btn" id="new-r">＋ Rechnung</button></div>
    <div class="card">${raw(docs.length ? html`<table class="tbl"><tr><th>Nummer</th><th>Art</th><th>Kunde</th><th>Titel</th><th>Datum</th><th>Betrag</th><th>Status</th></tr>
      ${docs.map(d => raw(html`<tr style="cursor:pointer" data-open="${d.id}"><td><b>${d.number || '–'}</b></td><td>${d.type === 'angebot' ? 'Angebot' : 'Rechnung'}</td>
        <td>${d.client_name || '–'}</td><td class="small">${d.title || ''}</td><td class="small">${d.issue_date}</td><td>${eur(d.total)}</td><td>${raw(st(d.status))}</td></tr>`))}</table>`
      : '<div class="empty"><div class="e">🧾</div>Noch keine Dokumente. Tipp: Im Lead öffnest du mit „Angebot erstellen“ direkt ein vorausgefülltes Angebot.</div>')}</div>`;
  const reload = () => tabDocs(el);
  el.querySelector('#new-a').addEventListener('click', async () => { const d = await api('/api/money/documents', { method: 'POST', body: { type: 'angebot' } }); openDoc(d.id, reload); });
  el.querySelector('#new-r').addEventListener('click', async () => { const d = await api('/api/money/documents', { method: 'POST', body: { type: 'rechnung' } }); openDoc(d.id, reload); });
  el.querySelectorAll('[data-open]').forEach(r => r.addEventListener('click', () => openDoc(Number(r.dataset.open), reload)));
}

async function openDoc(id, onClose) {
  document.querySelector('.drawer-bg')?.remove(); document.querySelector('.drawer')?.remove();
  const bg = document.createElement('div'); bg.className = 'drawer-bg';
  const dr = document.createElement('div'); dr.className = 'drawer'; dr.style.width = 'min(720px, 100vw)';
  document.body.append(bg, dr);
  const close = () => { bg.remove(); dr.remove(); onClose?.(); };
  bg.addEventListener('click', close);
  const render = async () => {
    const d = await api('/api/money/documents/' + id);
    const locked = d.type === 'rechnung' && d.number && d.status !== 'entwurf';
    const dis = locked ? 'disabled' : '';
    dr.innerHTML = html`
      <div class="row"><h2 style="margin:0">${d.type === 'angebot' ? 'Angebot' : 'Rechnung'} ${d.number || '(Entwurf)'}</h2>${raw(st(d.status))}<span class="spacer"></span><button class="btn sm icon" id="x">✕</button></div>
      ${raw(locked ? '<p class="small muted">Versendete Rechnungen sind gesperrt (Nachvollziehbarkeit). Bei Fehlern: stornieren und neu ausstellen.</p>' : '')}
      <div class="card flat form"><div class="fields">
        <label><span>Kunde</span><input data-k="client_name" value="${d.client_name || ''}" ${raw(dis)}></label>
        <label><span>E-Mail</span><input data-k="client_email" value="${d.client_email || ''}" ${raw(dis)}></label>
        <label style="grid-column:1/-1"><span>Anschrift</span><input data-k="client_address" value="${d.client_address || ''}" ${raw(dis)}></label>
        <label style="grid-column:1/-1"><span>Titel / Projekt</span><input data-k="title" value="${d.title || ''}" placeholder="z.B. Musikvideo „Song“ / Food-Content Dezember" ${raw(dis)}></label>
        <label><span>Datum</span><input type="date" data-k="issue_date" value="${d.issue_date || ''}" ${raw(dis)}></label>
        <label><span>Leistungszeitraum</span><input data-k="delivery" value="${d.delivery || ''}" placeholder="z.B. 12.12.2026" ${raw(dis)}></label>
        ${raw(d.type === 'rechnung' ? html`<label><span>Zahlbar bis</span><input type="date" data-k="due_date" value="${d.due_date || ''}" ${raw(dis)}></label>` : html`<label><span>Gültig bis</span><input type="date" data-k="valid_until" value="${d.valid_until || ''}" ${raw(dis)}></label>`)}
      </div></div>
      <div class="card flat"><h3>Positionen</h3>
        <table class="tbl" id="items"><tr><th>Leistung</th><th style="width:70px">Menge</th><th style="width:110px">Preis €</th><th style="width:100px" class="c">Summe</th><th></th></tr>
          ${d.items.map((it, i) => raw(html`<tr data-i="${i}"><td><input data-f="desc" value="${it.desc || ''}" ${raw(dis)}></td><td><input data-f="qty" type="number" step="0.5" value="${it.qty ?? 1}" ${raw(dis)}></td>
            <td><input data-f="price" type="number" step="0.01" value="${it.price ?? 0}" ${raw(dis)}></td><td class="c">${eur((it.qty || 0) * (it.price || 0))}</td><td>${raw(locked ? '' : '<button class="btn sm ghost" data-rm>✕</button>')}</td></tr>`))}
          <tr class="total"><td colspan="3" style="text-align:right"><b>Gesamt</b></td><td class="c"><b>${eur(d.total)}</b></td><td></td></tr></table>
        ${raw(locked ? '' : html`<div class="row" style="margin-top:8px"><select id="preset" style="max-width:420px"><option value="">＋ Vorlage hinzufügen …</option>${META.presets.map((p, i) => raw(html`<option value="${i}">${p.desc} – ${eur(p.price)}</option>`))}</select>
          <button class="btn sm" id="add-item">＋ Leere Position</button></div>`)}
        <p class="small muted">${META.ku ? 'Kleinunternehmer: Rechnung ohne USt. mit Hinweis §6 Abs. 1 Z 27 UStG.' : 'Preise inkl. 20 % USt.'}</p></div>
      <div class="card flat form">
        <label><span>Einleitung (optional)</span><textarea data-k="intro" style="min-height:60px" ${raw(dis)}>${d.intro || ''}</textarea></label>
        <label><span>Nutzungsrechte</span><textarea data-k="rights" style="min-height:80px" ${raw(dis)}>${d.rights || ''}</textarea></label>
        ${raw(locked ? '' : html`<div class="row"><button class="btn sm ghost" data-rights="artist">Rechte: Musikvideo</button><button class="btn sm ghost" data-rights="restaurant">Rechte: Restaurant</button></div>`)}
        <label style="margin-top:10px"><span>Anmerkung</span><textarea data-k="notes" style="min-height:50px" ${raw(dis)}>${d.notes || ''}</textarea></label>
      </div>
      <div class="row">
        ${raw(locked ? '' : '<button class="btn primary" id="save">Speichern</button>')}
        <a class="btn" href="/api/money/documents/${d.id}/print" target="_blank" rel="noopener">Vorschau / PDF ↗︎</a>
        ${raw(d.status === 'entwurf' ? '<button class="btn" data-st="gesendet">Als gesendet markieren</button>' : '')}
        ${raw(d.type === 'angebot' && ['gesendet', 'entwurf'].includes(d.status) ? '<button class="btn green" data-st="angenommen">Angenommen 🎉</button><button class="btn red" data-st="abgelehnt">Abgelehnt</button>' : '')}
        ${raw(d.type === 'angebot' && d.status !== 'abgelehnt' ? '<button class="btn" id="to-inv">→ Rechnung erstellen</button>' : '')}
        ${raw(d.type === 'rechnung' && d.status === 'gesendet' ? '<button class="btn green" data-st="bezahlt">Bezahlt</button>' : '')}
        ${raw(d.type === 'rechnung' && d.number && d.status !== 'storniert' ? '<button class="btn red" data-st="storniert">Stornieren</button>' : '')}
        <span class="spacer"></span>${raw(!(d.type === 'rechnung' && d.number) ? '<button class="btn sm ghost" id="del">Löschen</button>' : '')}
      </div>
      <p class="small muted" style="margin-top:12px">Nummer wird beim Senden vergeben (lückenlos: A-/R-Jahr-001). Bezahlte Rechnungen landen automatisch in den Einnahmen. ${d.lead_id ? 'Mit Lead verknüpft – Status wird im CRM mitgeführt.' : ''}</p>`;
    dr.querySelector('#x').addEventListener('click', close);
    const collect = () => {
      const body = { type: d.type, lead_id: d.lead_id };
      dr.querySelectorAll('[data-k]').forEach(i => { body[i.dataset.k] = i.value; });
      body.items = [...dr.querySelectorAll('#items tr[data-i]')].map(tr => ({ desc: tr.querySelector('[data-f=desc]').value, qty: Number(tr.querySelector('[data-f=qty]').value), price: Number(tr.querySelector('[data-f=price]').value) }));
      return body;
    };
    const save = async (extra = {}) => { const b = collect(); Object.assign(b, extra); await api('/api/money/documents/' + id, { method: 'PUT', body: b }); };
    dr.querySelector('#save')?.addEventListener('click', async () => { await save(); toast('Gespeichert', 'ok'); render(); });
    dr.querySelector('#preset')?.addEventListener('change', async e => { const p = META.presets[e.target.value]; const b = collect(); b.items.push({ desc: p.desc, qty: 1, price: p.price }); await api('/api/money/documents/' + id, { method: 'PUT', body: b }); render(); });
    dr.querySelector('#add-item')?.addEventListener('click', async () => { const b = collect(); b.items.push({ desc: '', qty: 1, price: 0 }); await api('/api/money/documents/' + id, { method: 'PUT', body: b }); render(); });
    dr.querySelectorAll('[data-rm]').forEach(btn => btn.addEventListener('click', async () => { const b = collect(); b.items.splice(Number(btn.closest('tr').dataset.i), 1); await api('/api/money/documents/' + id, { method: 'PUT', body: b }); render(); }));
    dr.querySelectorAll('#items input').forEach(i => i.addEventListener('change', async () => { await save(); render(); }));
    dr.querySelectorAll('[data-rights]').forEach(b => b.addEventListener('click', () => { dr.querySelector('[data-k=rights]').value = META.rights[b.dataset.rights]; }));
    dr.querySelectorAll('[data-st]').forEach(b => b.addEventListener('click', async () => {
      if (!locked && d.status === 'entwurf') await save();
      if (b.dataset.st === 'storniert' && !confirm('Rechnung stornieren?')) return;
      await api(`/api/money/documents/${id}/status`, { method: 'POST', body: { status: b.dataset.st } }); toast('Status aktualisiert', 'ok'); render();
    }));
    dr.querySelector('#to-inv')?.addEventListener('click', async () => { await save(); const inv = await api(`/api/money/documents/${id}/to-invoice`, { method: 'POST' }); id = inv.id; toast('Rechnung erstellt', 'ok'); render(); });
    dr.querySelector('#del')?.addEventListener('click', async () => { if (confirm('Dokument löschen?')) { await api('/api/money/documents/' + id, { method: 'DELETE' }); close(); } });
  };
  render();
}

async function tabLedger(el) {
  const rows = await api('/api/money/ledger');
  const cats = { einnahme: ['Video/Foto', 'Beat-Verkauf (BeatStars)', 'YouTube', 'Sonstiges'], ausgabe: ['Equipment', 'Software/Abos', 'Fahrtkosten', 'Werbung', 'Samples/Plugins', 'Sonstiges'] };
  el.innerHTML = html`
    <div class="card form" style="margin-top:14px"><h3>Buchung hinzufügen</h3><div class="fields">
      <label><span>Datum</span><input type="date" id="l-date" value="${new Date().toISOString().slice(0, 10)}"></label>
      <label><span>Art</span><select id="l-kind"><option value="ausgabe">Ausgabe</option><option value="einnahme">Einnahme</option></select></label>
      <label><span>Kategorie</span><select id="l-cat"></select></label>
      <label><span>Betrag €</span><input id="l-amt" type="number" step="0.01"></label>
      <label style="grid-column:1/-1"><span>Beschreibung</span><input id="l-desc" placeholder="z.B. Sony 35mm GM / BeatStars Auszahlung Nov."></label></div>
      <div class="row" style="margin-top:10px"><button class="btn primary" id="l-add">Hinzufügen</button><span class="small muted">Bezahlte Rechnungen werden automatisch gebucht. Belege trotzdem aufheben (7 Jahre).</span></div></div>
    <div class="card">${raw(rows.length ? html`<table class="tbl"><tr><th>Datum</th><th>Kategorie</th><th>Beschreibung</th><th>Betrag</th><th></th></tr>${rows.map(r => raw(html`<tr><td class="small">${r.date}</td><td>${r.category || '–'}</td><td>${r.description || ''}</td>
      <td class="${r.kind === 'einnahme' ? 'pos' : ''}">${r.kind === 'einnahme' ? '+' : '−'} ${eur(r.amount)}</td><td>${raw(r.document_id ? '<span class="badge gray">Rechnung</span>' : html`<button class="btn sm ghost" data-del="${r.id}">✕</button>`)}</td></tr>`))}</table>` : '<p class="muted small">Noch keine Buchungen dieses Jahr.</p>')}</div>`;
  const kindSel = el.querySelector('#l-kind'), catSel = el.querySelector('#l-cat');
  const fill = () => { catSel.innerHTML = cats[kindSel.value].map(c => `<option>${c}</option>`).join(''); };
  kindSel.addEventListener('change', fill); fill();
  el.querySelector('#l-add').addEventListener('click', async () => {
    const amount = Number(el.querySelector('#l-amt').value);
    if (!amount) return toast('Betrag fehlt', 'err');
    await api('/api/money/ledger', { method: 'POST', body: { date: el.querySelector('#l-date').value, kind: kindSel.value, category: catSel.value, description: el.querySelector('#l-desc').value, amount } });
    toast('Gebucht', 'ok'); tabLedger(el);
  });
  el.querySelectorAll('[data-del]').forEach(b => b.addEventListener('click', async () => { await api('/api/money/ledger/' + b.dataset.del, { method: 'DELETE' }); tabLedger(el); }));
}
