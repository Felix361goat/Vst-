// Portfolio pages: manage works per target group, preview, export as a single HTML file.
import { api, html, raw, toast, store } from '/static/api.js';

export default {
  async mount(el) {
    let niche = store('portfolio.niche') || 'musikvideo';
    const draw = async () => {
      const { items, niches } = await api('/api/portfolio/items?niche=' + niche);
      el.innerHTML = html`
        <div class="row" style="align-items:flex-end"><div><h1>Portfolio-Seiten</h1><p class="lead">Eine fokussierte Seite pro Zielgruppe – als Link in DMs, Briefen (QR-Code) und Mails.</p></div>
          <span class="spacer"></span><div class="tabs" id="n">${Object.entries(niches).map(([k, l]) => raw(html`<button data-n="${k}" class="${k === niche ? 'active' : ''}">${l}</button>`))}</div></div>
        <div class="grid2">
          <div class="card form"><h3>Projekt hinzufügen</h3>
            <label><span>Titel</span><input id="t" placeholder="${niche === 'food' ? 'z.B. Brunch-Shooting' : 'z.B. „Song“ – Musikvideo'}"></label>
            <label><span>Kunde</span><input id="c" placeholder="${niche === 'food' ? 'Café Muster, Mödling' : 'Artist-Name'}"></label>
            <label><span>Video-Link (YouTube wird eingebettet; Instagram/TikTok als Link)</span><input id="u" placeholder="https://youtu.be/…"></label>
            <label><span>Bild-URL (optional, wenn kein YouTube-Video)</span><input id="i" placeholder="https://…jpg"></label>
            <label><span>Ergebnis (wirkt am stärksten!)</span><input id="r" placeholder="${niche === 'food' ? 'z.B. +40 % Bestellungen auf Wolt im ersten Monat' : 'z.B. 120k Views in 2 Wochen'}"></label>
            <button class="btn primary" id="add">Hinzufügen</button></div>
          <div class="card"><h3>Seite</h3>
            <p class="small muted">Name, Instagram, E-Mail und Ort kommen aus den Einstellungen. Pakete & „ab“-Preise sind vorausgefüllt.</p>
            <div class="row"><a class="btn primary" href="/api/portfolio/preview/${niche}" target="_blank" rel="noopener">Vorschau ↗︎</a>
              <a class="btn" href="/api/portfolio/export/${niche}">Als HTML-Datei exportieren</a></div>
            <ol class="small" style="margin-top:14px"><li>Exportieren → Datei heißt <code>index-${niche}.html</code>, in <code>index.html</code> umbenennen.</li>
              <li>Auf <a href="https://app.netlify.com/drop" target="_blank" rel="noopener">Netlify Drop</a> ziehen (gratis) – du bekommst einen Link.</li>
              <li>Link unter Einstellungen als <b>Portfolio-Link</b> eintragen → steckt automatisch in allen Vorlagen ({portfolio}).</li></ol></div>
        </div>
        <div class="card"><h3>${items.length} Projekte</h3>${raw(items.length ? html`<table class="tbl">${items.map(x => raw(html`<tr>
          <td><input type="checkbox" data-vis="${x.id}" ${raw(x.visible ? 'checked' : '')} title="sichtbar"></td>
          <td><b>${x.title}</b><div class="lead-sub">${x.client}</div></td><td class="small">${x.result || ''}</td>
          <td class="small"><a href="${x.url || '#'}" target="_blank" rel="noopener">${x.url ? 'Link ↗︎' : ''}</a></td>
          <td><button class="btn sm ghost" data-del="${x.id}"></button></td></tr>`))}</table>` : '<p class="small muted">Noch keine Projekte – auch 2–3 starke Arbeiten reichen für den Anfang.</p>')}</div>`;
      el.querySelectorAll('[data-n]').forEach(b => b.addEventListener('click', () => { niche = b.dataset.n; store('portfolio.niche', niche); draw(); }));
      el.querySelector('#add').addEventListener('click', async () => {
        const v = id => el.querySelector(id).value.trim();
        if (!v('#t')) return toast('Titel fehlt', 'err');
        await api('/api/portfolio/items', { method: 'POST', body: { niche, title: v('#t'), client: v('#c'), url: v('#u'), image: v('#i'), result: v('#r') } });
        toast('Hinzugefügt', 'ok'); draw();
      });
      el.querySelectorAll('[data-del]').forEach(b => b.addEventListener('click', async () => { await api('/api/portfolio/items/' + b.dataset.del, { method: 'DELETE' }); draw(); }));
      el.querySelectorAll('[data-vis]').forEach(cb => cb.addEventListener('change', async () => {
        const it = items.find(x => x.id == cb.dataset.vis);
        await api('/api/portfolio/items/' + it.id, { method: 'PUT', body: { ...it, visible: cb.checked ? 1 : 0 } });
      }));
    };
    draw();
  },
};
