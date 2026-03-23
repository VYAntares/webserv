# PROCESS — Webserv Study Pages

> À relire à chaque nouvelle page. Ce doc décrit exactement comment les pages sont construites, ce qui est commun à toutes, et les pièges à ne pas répéter.

---

## Vue d'ensemble du projet

```
webserv-studies/
├── webserv-mindmap.html          ← carte centrale, point d'entrée unique
├── 01-sockets-tcp/               ← socket-lifecycle, network-addressing, recv-send, fcntl-nonblocking, multi-port
├── 02-io-multiplexing/           ← select-poll-epoll, epoll, epoll-wait-before-io, event-loop
├── 03-http-protocol/             ← http-requete, http-methods, http-response
├── 07-architecture/              ← full-architecture
└── PROCESS.md                    ← ce fichier
```

**Règle de nommage :** `NN-nom-kebab/` pour les dossiers. Les numéros correspondent aux sections du mindmap.

---

## 1. Ajouter une carte dans le mindmap

Dans `.concepts-grid`, ajouter une carte **avec `data-page`** pour la rendre cliquable :

```html
<div class="card" data-page="NN-dossier/fichier.html"
     style="--card-accent: var(--accent-X); --card-accent-dim: var(--accent-X-dim);">
  <span class="card-tag">Catégorie</span>
  <h3><a href="NN-dossier/fichier.html" style="text-decoration: none;">Titre</a></h3>
  <p>Description courte.</p>
  <div class="card-links">
    <a class="card-link" href="NN-dossier/fichier.html">Voir le cours →</a>
  </div>
</div>
```

**⚠️ Piège fréquent — couleur du titre au hover :**
Le lien `h3 > a` ne doit **pas** avoir `color: inherit` en style inline.
Avec `color: inherit`, le hover coloré est écrasé par l'inline style et ne fonctionne pas.
→ Mettre uniquement `style="text-decoration: none;"`.

**Comportement automatique** (JS déjà présent dans le mindmap) :
- Toute la carte devient cliquable dès que `data-page` est présent
- La position de scroll est sauvegardée avant navigation
- Fade-out 140ms avant la navigation

---

## 2. Navigation — toujours le pattern `progress-bar`

**Standard unique pour toutes les pages de contenu.** Barre sticky en haut avec le bouton retour à gauche, puis des pills pour naviguer entre les sections.

```html
<div class="progress-bar">
  <div class="progress-inner">
    <a href="../webserv-mindmap.html" class="back-link" id="back-btn">← Retour à la cartographie</a>
    <div class="progress-back-divider"></div>
    <a href="#section-1" class="step-pill active" id="nav-section-1"><span class="step-dot"></span>Titre</a>
    <div class="step-connector"></div>
    <a href="#section-2" class="step-pill" id="nav-section-2"><span class="step-dot"></span>Titre</a>
    <!-- ... -->
  </div>
</div>

<div class="header">
  <div class="header-tag">NN — Nom de section</div>
  <h1>Titre principal<br><em>italique coloré</em></h1>
  <p class="header-desc">Description. <code>code inline</code> si besoin.</p>
</div>
```

**CSS clé :**
```css
.progress-bar { position: sticky; top: 0; z-index: 100; background: rgba(10,10,15,0.9); backdrop-filter: blur(20px); border-bottom: 1px solid var(--border); padding: 0.75rem 2rem; }
.progress-inner { max-width: 900px; margin: 0 auto; display: flex; align-items: center; }
/* ⚠️ PAS de overflow-x: auto — la barre ne doit pas être scrollable horizontalement */
.step-pill { font-family: 'Space Mono', monospace; font-size: 0.6rem; text-transform: uppercase; letter-spacing: 0.1em; padding: 0.35rem 0.8rem; border-radius: 100px; color: var(--text-muted); text-decoration: none; white-space: nowrap; flex-shrink: 0; }
/* ⚠️ Taille des pills : NE JAMAIS réduire font-size ou padding pour faire tenir les pills.
   Si ça déborde à droite, c'est normal. Les pills sont en white-space: nowrap et dépassent naturellement. */
.step-pill.active { background: rgba(250,204,21,0.1); color: var(--accent-5); }
.step-connector { flex: 1; height: 1px; background: var(--border); min-width: 6px; }
.header { padding: 3rem 2rem 2rem; max-width: 900px; margin: 0 auto; border-bottom: 1px solid var(--border); }
.header h1 { font-family: 'Instrument Serif', serif; font-size: clamp(2rem, 5vw, 3.5rem); font-weight: 400; line-height: 1.1; }
.header h1 em { font-style: italic; color: var(--accent-X); }
.header-tag { font-family: 'Space Mono', monospace; font-size: 0.6rem; text-transform: uppercase; letter-spacing: 0.3em; border-radius: 100px; display: inline-block; margin-bottom: 1.2rem; }
```

**⚠️ top-nav (07-architecture) — exception unique**
La page `full-architecture.html` utilise un `top-nav` avec dropdowns car c'est une page-schéma spéciale. Ne pas reproduire ce pattern sur les pages de cours normales.

---

## 3. Hero section (page-hero)

Toujours centré, degrés de fond **non clippés** :

```html
<div class="page-hero">
  <div class="page-hero-inner">
    <div class="hero-tag">NN — Nom de section</div>
    <h1 class="hero-title">Titre avec <em>italique coloré</em></h1>
    <p class="hero-desc">Description de la page.</p>
  </div>
</div>
```

```css
.page-hero { padding: 5rem 2rem 4rem; text-align: center; overflow: visible; position: relative; z-index: 1; }
.page-hero-inner { max-width: 800px; margin: 0 auto; }
```

**⚠️ Piège — gradient coupé :**
`overflow: hidden` sur `.page-hero` coupe le `::before` pseudo-élément qui fait le glow.
→ Toujours `overflow: visible` sur page-hero. Les globs de fond sont maintenant en `position: fixed` séparés (voir §5).

---

## 4. Titres de chapitres — style epoll (pattern actuel)

Copier exactement ce CSS et ce HTML. C'est le style de `epoll.html` qu'on applique maintenant à toutes les nouvelles pages.

```css
.ch-header { display: flex; align-items: flex-start; gap: 1.5rem; margin-bottom: 2.5rem; }
.ch-number {
  font-family: 'Instrument Serif', serif;
  font-size: 4rem; line-height: 1; opacity: 0.15; flex-shrink: 0;
  color: var(--step-color, var(--accent-5));
}
.ch-fn {
  font-family: 'Space Mono', monospace; font-size: 1.4rem;
  font-weight: 700; color: var(--step-color, var(--accent-5)); margin-bottom: 0.3rem;
}
.ch-subtitle {
  font-family: 'Instrument Serif', serif; font-size: 1.8rem; font-weight: 400; color: var(--text);
}
```

```html
<section class="chapter" id="mon-chapitre" style="--step-color: var(--accent-2); scroll-margin-top: 80px;">
  <div class="ch-header">
    <div class="ch-number">01</div>
    <div>
      <div class="ch-fn">nomDeLaFonction()</div>
      <div class="ch-subtitle">Sous-titre élégant en serif</div>
    </div>
  </div>
  <!-- contenu -->
</section>
```

**⚠️ Piège — ancien style :**
L'ancien style utilisait `font-family: 'Space Mono'` pour le numéro, `font-size: 1.5rem`, `opacity: 0.4`.
→ Ne plus utiliser. Le style actuel : `Instrument Serif`, `4rem`, `opacity: 0.15`.

---

## 5. Background globs de couleur

**Obligatoire sur TOUTES les pages — pages de contenu ET `webserv-mindmap.html`.**

Ajouter 2-3 blobs fixes en fond pour l'ambiance. Les mettre juste après `<body>` :

```html
<div class="bg-glow bg-glow-1"></div>
<div class="bg-glow bg-glow-2"></div>
<div class="bg-glow bg-glow-3"></div>
```

```css
.bg-glow { position: fixed; border-radius: 50%; pointer-events: none; z-index: 0; filter: blur(80px); }
.bg-glow-1 { width: 700px; height: 700px; top: -200px; left: -150px; background: radial-gradient(ellipse, rgba(250,204,21,0.05) 0%, transparent 70%); }
.bg-glow-2 { width: 600px; height: 600px; top: 40vh; right: -200px; background: radial-gradient(ellipse, rgba(74,224,255,0.05) 0%, transparent 70%); }
.bg-glow-3 { width: 500px; height: 500px; bottom: 10vh; left: 20%; background: radial-gradient(ellipse, rgba(192,132,252,0.05) 0%, transparent 70%); }
```

Tous les éléments de contenu (nav, hero, main, footer) doivent avoir `position: relative; z-index: 1` pour s'afficher au-dessus.

Les couleurs des blobs peuvent varier selon la page pour s'adapter à l'accent dominant, l'important est que les opacités restent très basses (0.04–0.06).

---

## 6. Sub-IDs pour navigation précise depuis dropdowns

Quand un dropdown pointe vers une sous-section à l'intérieur d'un chapitre, ajouter un div vide avec l'ID avant le contenu cible :

```html
<div id="ch-static" style="scroll-margin-top: 80px;"></div>
<p class="prose"><strong>StaticHandler...</strong></p>
```

Pour les éléments inline :
```html
<div id="ch-epollin" style="display:inline; scroll-margin-top: 80px;"></div><strong>Quand EPOLLIN...</strong>
```

**⚠️ Piège — scroll-margin-top oublié :**
Sans `scroll-margin-top: 80px`, le nav sticky masque le début de la section lors du scroll automatique.

---

## 7. Smooth scroll JS (top-nav pattern)

Toujours utiliser cette implémentation pour le pattern top-nav (pas `scrollIntoView` sans offset) :

```js
function scrollToId(id) {
  if (id === 'top') { window.scrollTo({ top: 0, behavior: 'smooth' }); return; }
  var el = document.getElementById(id);
  if (!el) return;
  var navH = document.querySelector('.top-nav').offsetHeight;
  var top = el.getBoundingClientRect().top + window.pageYOffset - navH - 16;
  window.scrollTo({ top: top, behavior: 'smooth' });
}
document.querySelectorAll('a[href^="#"]').forEach(function(el) {
  el.addEventListener('click', function(e) {
    var href = this.getAttribute('href');
    if (href.length < 2) return;
    e.preventDefault();
    scrollToId(href.slice(1));
  });
});
```

**⚠️ Piège — `scrollIntoView` sans offset :**
`element.scrollIntoView()` ne tient pas compte de la hauteur du nav sticky.
→ Toujours utiliser `scrollToId()` qui soustrait `navH`.

---

## 8. Footer obligatoire

Toutes les pages ont ce footer identique (adapter la couleur du lien à l'accent de la page) :

```html
<footer class="footer">
  <p>← <a href="../webserv-mindmap.html">Retour à la cartographie Webserv</a></p>
  <p style="margin-top:0.5rem; opacity:0.4">42 Project — Document d'étude — NN Description courte</p>
</footer>
```

```css
.footer { text-align: center; padding: 3rem 2rem; border-top: 1px solid var(--border); margin-top: 4rem; position: relative; z-index: 1; }
.footer p { font-family: 'Space Mono', monospace; font-size: 0.65rem; color: var(--text-muted); letter-spacing: 0.1em; }
.footer a { color: var(--accent-X); text-decoration: none; }
```

---

## 9. Transitions page (JS obligatoire partout)

```js
// Fade-in au chargement
document.body.style.opacity = '0';
document.body.style.transition = 'opacity 0.14s ease';
window.addEventListener('DOMContentLoaded', function() {
  requestAnimationFrame(function() { document.body.style.opacity = '1'; });
});

// Fade-out sur tous les liens vers le mindmap
function navigateTo(href) {
  document.body.style.opacity = '0';
  setTimeout(function() { window.location.href = href; }, 140);
}
document.querySelectorAll('a[href="../webserv-mindmap.html"]').forEach(function(a) {
  a.addEventListener('click', function(e) { e.preventDefault(); navigateTo(a.href); });
});
```

**Durée : 140ms, sans exception.**

---

## 10. Convention epoll (jamais poll/select)

On travaille **exclusivement avec epoll**.

| À éviter | Remplacer par |
|----------|--------------|
| `poll()` | `epoll_wait()` |
| `POLLIN` | `EPOLLIN` |
| `POLLOUT` | `EPOLLOUT` |
| `POLLERR` | `EPOLLERR` |
| `POLLHUP` | `EPOLLHUP` |
| `struct pollfd` | `struct epoll_event` |

Les 3 appels epoll : `epoll_create1()` (créer), `epoll_ctl()` (ajouter/modifier/supprimer), `epoll_wait()` (attendre).

**Exception :** Citations verbatim du PDF sujet conservées telles quelles entre `«»`.

---

## 11. recv/send — toujours Edge Triggered

Le code de recv/send utilise **ET (Edge Triggered)**, pas LT :

- `handle_recv` : boucle `while(true)` → `recv()` → break sur `EAGAIN/EWOULDBLOCK`
- `handle_send` : boucle `while(sent < total)` → `send()` → re-register `EPOLLOUT|EPOLLET` sur `EAGAIN`
- Transition après recv : `ev.events = EPOLLOUT | EPOLLET`

---

## 12. Pages existantes et leurs cartes mindmap

| Section mindmap | Carte | Fichier |
|----------------|-------|---------|
| 01 Sockets | Le cycle de vie d'un socket serveur | `01-sockets-tcp/socket-lifecycle.html` |
| 01 Sockets | Adressage & options de socket | `01-sockets-tcp/network-addressing.html` |
| 01 Sockets | Envoi & réception de données | `01-sockets-tcp/recv-send.html` |
| 01 Sockets | Blocking vs Non-blocking | `01-sockets-tcp/fcntl-nonblocking.html` |
| 01 Sockets | Écouter sur plusieurs ports | `01-sockets-tcp/multi-port.html` |
| 02 I/O Multiplexing | Pourquoi epoll plutôt que poll ou select | `02-io-multiplexing/select-poll-epoll.html` |
| 02 I/O Multiplexing | epoll — Le surveillant événementiel | `02-io-multiplexing/epoll.html` |
| 02 I/O Multiplexing | La règle d'or : toujours epoll_wait() avant read/write | `02-io-multiplexing/epoll-wait-before-io.html` |
| 02 I/O Multiplexing | La boucle événementielle (Event Loop) | `02-io-multiplexing/event-loop.html` |
| 03 HTTP | Structure d'une requête HTTP | `03-http-protocol/http-requete.html` |
| 03 HTTP | GET, POST, DELETE — Les méthodes HTTP | `03-http-protocol/http-methods.html` |
| 03 HTTP | Status Codes & Headers — La réponse HTTP | `03-http-protocol/http-response.html` |
| 07 Architecture | Le parcours d'une requête | `07-architecture/full-architecture.html` |

---

## Checklist avant de valider une nouvelle page

- [ ] Carte dans `webserv-mindmap.html` avec `data-page="..."`
- [ ] `h3 > a` sans `color: inherit` inline (sinon hover cassé)
- [ ] Nav sticky `progress-bar` (PAS top-nav) centré, pas de `overflow-x: auto`
- [ ] Hero section : pattern `.header` standard (tag + h1 + header-desc), PAS page-hero
- [ ] Titres chapitres style epoll : `Instrument Serif` 4rem / Space Mono 1.4rem / Serif 1.8rem
- [ ] **3 bg-glow fixes en fond** (obligatoire sur toutes les pages, y compris webserv-mindmap.html)
- [ ] Pills `progress-bar` : font-size 0.6rem, padding 0.35rem 0.8rem — **jamais réduire pour faire tenir**
- [ ] `scroll-margin-top: 80px` sur tous les IDs ciblés par le nav
- [ ] Smooth scroll JS avec offset `.progress-bar` (pas `.top-nav`)
- [ ] Footer `← Retour à la cartographie Webserv`
- [ ] Transitions fade 140ms (fade-in + fade-out sur lien retour)
- [ ] Aucun `poll()`, `POLLIN`, `POLLOUT` — uniquement les équivalents epoll
- [ ] recv/send en ET avec boucles jusqu'à EAGAIN
