# Processus — Ajout de pages HTML à webserv-mindmap

**À relire à chaque nouvelle page créée ensemble.**

---

## Principe général

`webserv-mindmap.html` est la carte centrale du projet. Chaque carte cliquable de cette page pointe vers une page HTML dédiée qui approfondit un concept. Chaque nouvelle page doit s'intégrer de manière cohérente : même navigation, même design system, mêmes transitions.

---

## 1. Ajouter la carte dans `webserv-mindmap.html`

Dans la grille `.concepts-grid`, ajouter une carte avec l'attribut `data-page` pointant vers le fichier HTML cible :

```html
<div class="card" data-page="nom-de-la-page.html" style="--card-accent: var(--accent-X); --card-accent-dim: var(--accent-X-dim);">
  <span class="card-tag">Catégorie</span>
  <h3><a href="nom-de-la-page.html">Titre de la page</a></h3>
  <p>Description courte du concept.</p>
  <div class="card-functions">
    <span class="fn-tag">fonction()</span>
  </div>
  <div class="card-links">
    <a class="card-link" href="https://..." target="_blank">référence externe</a>
  </div>
</div>
```

**Comportement automatique** : dès que `data-page` est présent, le JS existant rend toute la carte cliquable (sauf les `.card-link`), sauvegarde la position de scroll, et déclenche le fade-out à 140ms avant de naviguer. Le titre passe à la couleur accent au hover de la carte.

---

## 2. Structure obligatoire de chaque nouvelle page HTML

### En-tête sticky (barre de navigation supérieure)

La page doit avoir une `progress-bar` sticky avec le bouton retour à gauche. Copier ce bloc exactement :

```html
<div class="progress-bar">
  <div class="progress-inner">
    <a href="webserv-mindmap.html" class="back-link" id="back-btn">← Retour à la cartographie</a>
    <div class="progress-back-divider"></div>
    <!-- Pills de navigation propres à la page -->
    <a href="#section-1" class="step-pill active" id="nav-section-1">
      <span class="step-dot"></span>Titre section
    </a>
    <!-- ... -->
  </div>
</div>
```

Le `.progress-bar` doit avoir `position: sticky; top: 0; z-index: 100;` — il reste visible en permanence lors du scroll.

### En-tête de page (non sticky, sous la progress-bar)

```html
<div class="header">
  <div class="header-tag">Catégorie — Sous-catégorie</div>
  <h1>Titre principal<br>avec <em>italique</em></h1>
  <p class="header-desc">Description de la page.</p>
</div>
```

### Footer (bas de page)

Toujours présent, toujours identique dans la structure :

```html
<footer class="footer">
  <p>← <a href="webserv-mindmap.html">Retour à la cartographie Webserv</a></p>
  <p style="margin-top:0.5rem; opacity:0.4">42 Project — Document d'étude — [description courte]</p>
</footer>
```

---

## 3. Transitions de page (JS obligatoire)

Chaque page doit inclure ce bloc JS au début de la balise `<script>` :

```js
// Page fade-in
document.body.style.opacity = '0';
document.body.style.transition = 'opacity 0.14s ease';
window.addEventListener('DOMContentLoaded', () => {
  requestAnimationFrame(() => { document.body.style.opacity = '1'; });
});

// Fade-out sur le bouton retour et le lien footer
function navigateTo(href) {
  document.body.style.opacity = '0';
  setTimeout(() => { window.location.href = href; }, 140);
}
document.querySelectorAll('a[href="webserv-mindmap.html"]').forEach(a => {
  a.addEventListener('click', e => {
    e.preventDefault();
    navigateTo(a.href);
  });
});
```

**Durée des transitions : 140ms partout, sans exception.**

---

## 4. CSS minimal requis

La page doit inclure les variables CSS du design system et les styles suivants pour l'en-tête sticky :

```css
.progress-bar {
  position: sticky;
  top: 0;
  z-index: 100;
  background: rgba(10,10,15,0.9);
  backdrop-filter: blur(20px);
  border-bottom: 1px solid var(--border);
  padding: 0.75rem 2rem;
}
.progress-inner {
  max-width: 900px;
  margin: 0 auto;
  display: flex;
  align-items: center;
  gap: 0;
}
.back-link {
  font-family: 'Space Mono', monospace;
  font-size: 0.6rem;
  text-transform: uppercase;
  letter-spacing: 0.2em;
  color: var(--text-muted);
  text-decoration: none;
  display: inline-flex;
  align-items: center;
  gap: 0.4rem;
  padding: 0.35rem 0.5rem;
  border-radius: 6px;
  white-space: nowrap;
  flex-shrink: 0;
  transition: color 0.2s, background 0.2s;
}
.back-link:hover { color: var(--accent-2); background: rgba(74,224,255,0.07); }
.progress-back-divider {
  width: 1px;
  height: 14px;
  background: var(--border);
  flex-shrink: 0;
  margin: 0 0.75rem;
}
```

Prendre `socket-lifecycle.html` comme référence complète pour le reste du CSS (variables, fonts, step-sections, code blocks, etc.).

---

## 5. Convention I/O Multiplexing — epoll

**Nous travaillons exclusivement avec `epoll` (pas `poll`, pas `select`).**

Dans toutes les pages et explications :
- `poll()` → `epoll_wait()` ou `epoll` selon le contexte
- `POLLIN` → `EPOLLIN`
- `POLLOUT` → `EPOLLOUT`
- `POLLERR` → `EPOLLERR`
- `POLLHUP` → `EPOLLHUP`
- `struct pollfd` → `struct epoll_event`
- Les trois appels epoll : `epoll_create1()` (création), `epoll_ctl()` (enregistrement/modification/suppression), `epoll_wait()` (attente d'événements)
- Les citations verbatim du sujet PDF (entre guillemets `«»` avec `— webserv.pdf`) mentionnent `poll() (or equivalent)` — les conserver telles quelles puisque c'est une citation officielle.

---

## 6. Checklist avant de valider une nouvelle page

- [ ] Carte dans `webserv-mindmap.html` avec `data-page="..."`
- [ ] `.progress-bar` sticky avec `← Retour à la cartographie` toujours visible
- [ ] `.header` avec `.header-tag`, `h1`, `.header-desc`
- [ ] `<footer>` avec lien retour et mention 42 Project
- [ ] JS de transition à 140ms présent
- [ ] Toutes les navigations vers `webserv-mindmap.html` passent par `navigateTo()`
- [ ] Aucune mention de `poll()` ou `POLLIN/OUT/ERR/HUP` — uniquement les équivalents epoll
