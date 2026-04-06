# 🌐 Webserv — Sujet complet (42, v24.0)

> *"This is when you finally understand why URLs start with HTTP"*

---

## Table des matières

- [Introduction](#introduction)
- [Règles générales](#règles-générales)
- [Instructions IA](#instructions-ia)
- [Partie obligatoire](#partie-obligatoire)
  - [Requirements techniques](#requirements-techniques)
  - [Spécificités macOS](#spécificités-macos)
  - [Fichier de configuration](#fichier-de-configuration)
- [README requis](#readme-requis)
- [Bonus](#bonus)
- [Rendu & évaluation](#rendu--évaluation)
- [Récapitulatif des pièges critiques](#récapitulatif-des-pièges-critiques)

---

## Introduction

Le protocole HTTP (Hypertext Transfer Protocol) est un protocole applicatif pour les systèmes d'information distribués, collaboratifs et hypermédia.

HTTP est la **fondation de la communication de données** du World Wide Web. Les documents hypertexte incluent des hyperliens vers d'autres ressources accessibles facilement par l'utilisateur (clic de souris, tap sur écran dans un navigateur).

**Rôle principal d'un serveur web :**
- Stocker, traiter et délivrer des pages web aux clients
- La communication client-serveur passe par HTTP
- Les pages délivrées sont le plus souvent des documents HTML (images, feuilles de style, scripts inclus)

**Caractéristiques importantes :**
- Plusieurs serveurs web peuvent être utilisés pour un site à fort trafic (répartition de charge entre machines physiques)
- Un *user agent* (navigateur ou crawler) initie la communication en demandant une ressource via HTTP
- Le serveur répond avec le contenu de la ressource ou un message d'erreur
- La ressource est typiquement un fichier réel sur le stockage du serveur, ou le résultat d'un programme
- HTTP permet aussi aux clients d'**envoyer des données** (formulaires web, upload de fichiers)

---

## Règles générales

| Règle | Détail |
|-------|--------|
| **Crash interdit** | Le programme ne doit jamais crasher ni se terminer de manière inattendue (même en cas de manque de mémoire). Si c'est le cas → note = **0** |
| **Makefile obligatoire** | Doit compiler les sources, sans relinkage inutile |
| **Règles Makefile** | `$(NAME)`, `all`, `clean`, `fclean`, `re` |
| **Compilation** | `c++` avec les flags `-Wall -Wextra -Werror` |
| **Standard C++** | C++ 98. Doit compiler avec `-std=c++98` |
| **Préférer C++** | Utiliser `<cstring>` plutôt que `<string.h>`, etc. Les fonctions C sont autorisées mais préférer leurs équivalents C++ |
| **Bibliothèques externes** | **Interdites** (Boost et toutes autres libs externes) |

---

## Instructions IA

> Le sujet intègre une section dédiée à l'usage de l'IA dans le cursus 42.

### Message principal

- ✅ Utiliser l'IA pour réduire les tâches répétitives ou fastidieuses
- ✅ Développer des compétences de *prompting* (coding et non-coding) utiles pour la carrière
- ✅ Apprendre comment fonctionnent les systèmes IA pour anticiper risques, biais et problèmes éthiques
- ✅ Continuer à construire les compétences techniques et *soft skills* avec les pairs
- ✅ N'utiliser que du contenu généré par IA que l'on **comprend totalement** et dont on peut prendre la responsabilité

### Règles pour l'apprenant

- Réfléchir au problème **avant** de prompter (prompt plus clair, plus précis)
- Systématiquement vérifier, revoir, questionner et tester tout ce qui est généré par IA
- Toujours demander une peer review

### ❌ Mauvaises pratiques à éviter

- Copier-coller une fonction entière générée par IA sans comprendre ce qu'elle fait → échec garanti à la peer-eval
- Laisser Copilot générer une partie clé du projet sans pouvoir l'expliquer

### ✅ Bonnes pratiques

- Utiliser l'IA pour concevoir un parser, puis revoir la logique avec un pair → corriger les bugs ensemble
- Demander à l'IA comment tester une fonction, essayer, puis revoir les résultats avec un pair

---

## Partie obligatoire

### Informations générales

| Champ | Valeur |
|-------|--------|
| **Nom du programme** | `webserv` |
| **Fichiers à rendre** | `Makefile`, `*.{h,hpp}`, `*.cpp`, `*.tpp`, `*.ipp`, fichiers de configuration |
| **Règles Makefile** | `NAME`, `all`, `clean`, `fclean`, `re` |
| **Arguments** | `[fichier de configuration]` (optionnel si chemin par défaut défini) |
| **Description** | Un serveur HTTP en C++ 98 |

### Fonctions externes autorisées

```
execve, pipe, strerror, gai_strerror, errno, dup, dup2, fork,
socketpair, htons, htonl, ntohs, ntohl, select, poll,
epoll (epoll_create, epoll_ctl, epoll_wait),
kqueue (kqueue, kevent), socket, accept, listen, send, recv,
chdir, bind, connect, getaddrinfo, freeaddrinfo, setsockopt,
getsockname, getprotobyname, fcntl, close, read, write,
waitpid, kill, signal, access, stat, open, opendir, readdir,
closedir
```

### Usage

```bash
./webserv [fichier_de_configuration]
```

> **Note :** Bien que `poll()` soit mentionné dans le sujet et la fiche d'évaluation, vous pouvez utiliser n'importe quelle fonction équivalente : `select()`, `kqueue()`, ou `epoll()`.

> **Recommandation :** Lire les RFC définissant le protocole HTTP et effectuer des tests avec `telnet` et NGINX **avant** de commencer. HTTP 1.0 est suggéré comme point de référence mais n'est pas imposé.

---

### Requirements techniques

#### Non-blocking I/O — ⚠️ CRITIQUE

- Le serveur doit rester **non-bloquant à tout moment**
- Gérer correctement les déconnexions clients
- Utiliser **un seul `poll()`** (ou équivalent) pour **toutes** les opérations I/O entre clients et serveur (y compris le `listen`)
- `poll()` doit surveiller **lecture ET écriture simultanément**
- **Ne jamais** faire de `read` ou `write` sans passer par `poll()` (ou équivalent)
- **Interdiction** de vérifier `errno` pour ajuster le comportement du serveur après une opération `read` ou `write`
- `poll()` (ou équivalent) **n'est pas requis** pour les fichiers disque ordinaires — `read()` et `write()` sur ces fichiers n'ont pas besoin de notifications de disponibilité

> ⚠️ **PIÈGE CRITIQUE :** Appeler `read`/`recv` ou `write`/`send` sur des sockets, pipes/FIFOs sans readiness préalable → **note = 0**

#### Comportement général

- Une requête au serveur ne doit **jamais rester en attente indéfiniment**
- Doit être compatible avec les navigateurs web standards (au choix)
- NGINX peut être utilisé pour comparer les headers et les comportements de réponse (attention aux différences entre versions HTTP)
- Les codes de statut HTTP doivent être **précis**
- Pages d'erreur par défaut si aucune n'est fournie
- **`fork` réservé aux CGI uniquement** (PHP, Python, etc.) — interdit pour autre usage
- Doit pouvoir servir un site **entièrement statique**
- Les clients doivent pouvoir **uploader des fichiers**
- Méthodes HTTP minimales : **GET, POST, DELETE**
- Stress-tester le serveur pour s'assurer qu'il reste disponible en permanence
- Doit pouvoir écouter sur **plusieurs ports** pour délivrer différents contenus

> **Note :** Le *virtual host* est considéré hors-scope dans ce contexte, mais son implémentation est autorisée.

---

### Spécificités macOS

> Cette section s'applique **uniquement** à macOS.

macOS gère `write()` différemment des autres OS Unix. Par conséquent :

- Utiliser `fcntl()` est autorisé
- Les file descriptors doivent être utilisés en **mode non-bloquant**
- `fcntl()` n'est autorisé qu'avec les flags suivants : `F_SETFL`, `O_NONBLOCK`, `FD_CLOEXEC`
- **Tout autre flag est interdit**

---

### Fichier de configuration

> S'inspirer de la section `server` du fichier de configuration NGINX.

#### Ce que le fichier de configuration doit permettre :

**1. Définir les paires interface:port d'écoute**
- Permet de définir plusieurs sites web servis par le programme

**2. Configurer les pages d'erreur par défaut**

**3. Définir la taille maximale du corps des requêtes client**

**4. Configurer des règles par URL/route** (pas de regex nécessaire) :

| Directive | Description |
|-----------|-------------|
| **Méthodes HTTP acceptées** | Lister les méthodes autorisées pour la route |
| **Redirection HTTP** | Configurer des redirections |
| **Racine du répertoire** | Définir où chercher les fichiers (ex: `/kapouet` → `/tmp/www`, donc `/kapouet/pouic/toto/pouet` → `/tmp/www/pouic/toto/pouet`) |
| **Listing de répertoire** | Activer ou désactiver l'affichage du contenu d'un répertoire |
| **Fichier par défaut** | Fichier à servir quand la ressource demandée est un répertoire |
| **Upload de fichiers** | Autoriser l'upload et définir l'emplacement de stockage |
| **CGI** | Exécution basée sur l'extension de fichier (ex: `.php`) |

#### Remarques spécifiques sur les CGI :

- Les **variables d'environnement** impliquées dans la communication serveur-CGI sont cruciales
- La **requête complète et les arguments** fournis par le client doivent être disponibles pour le CGI
- Pour les **requêtes chunked** : le serveur doit les **dé-chunker** ; le CGI attend `EOF` comme fin du body
- Pour la **sortie du CGI** : si aucun `content_length` n'est retourné, `EOF` marque la fin des données
- Le CGI doit être exécuté dans le **bon répertoire** pour l'accès aux fichiers par chemin relatif
- Le serveur doit supporter **au moins un CGI** (php-CGI, Python, etc.)

#### Obligations supplémentaires :

- Fournir des fichiers de configuration et des fichiers par défaut pour **tester et démontrer** que chaque fonctionnalité fonctionne lors de l'évaluation
- D'autres règles/informations peuvent être incluses (ex: nom de serveur pour virtual hosts si implémentés)

> En cas de doute sur un comportement spécifique, **comparer avec NGINX**.

> Un petit testeur est fourni — non obligatoire si tout fonctionne avec le navigateur, mais utile pour trouver et corriger des bugs.

> **Conseil :** Écrire les tests dans un langage adapté (Python, Golang, C, C++...), pas seulement avec un seul programme.

---

## README requis

Un fichier `README.md` **doit être présent** à la racine du dépôt Git.

### Sections obligatoires :

| Section | Contenu |
|---------|---------|
| **Première ligne** | Doit être en italique et lire : *This project has been created as part of the 42 curriculum by \<login1\>[, \<login2\>[, \<login3\>[...]]]* |
| **Description** | Présente clairement le projet, son objectif et un aperçu général |
| **Instructions** | Informations sur la compilation, l'installation et/ou l'exécution |
| **Resources** | Références classiques sur le sujet (docs, articles, tutoriels) + description de l'usage de l'IA (pour quelles tâches et quelles parties du projet) |

> Le README doit être **rédigé en anglais**.

---

## Bonus

> ⚠️ Le bonus n'est évalué que si la **partie obligatoire est entièrement complétée sans problèmes**.

Fonctionnalités bonus possibles :

- **Support des cookies et gestion de sessions** (fournir des exemples simples)
- **Gestion de plusieurs types de CGI**

---

## Rendu & évaluation

- Soumettre dans le dépôt Git habituel
- Seul le contenu du dépôt est évalué lors de la soutenance
- Vérifier soigneusement les noms des fichiers

### Modification lors de l'évaluation

Lors de l'évaluation, une **modification mineure du projet peut être demandée** :
- Changement de comportement mineur
- Quelques lignes de code à écrire ou réécrire
- Fonctionnalité simple à ajouter

> Cette étape vise à **vérifier la compréhension réelle** d'une partie spécifique du projet. La modification doit être réalisable en quelques minutes dans l'environnement de développement habituel.

---

## Récapitulatif des pièges critiques

> Section synthèse pour Claude Code — points d'attention maximale.

### 🔴 Note = 0 immédiate

| Piège | Règle |
|-------|-------|
| Crash ou terminaison inattendue | Le programme ne doit jamais crasher |
| `read`/`recv` ou `write`/`send` sur socket sans `poll()` préalable | Toujours passer par `poll()` |
| `fork` utilisé ailleurs que pour le CGI | Réservé exclusivement aux CGI |
| Vérifier `errno` après `read`/`write` pour ajuster le comportement | Strictement interdit |

### 🟠 Architecture non-blocking

- **1 seul** `poll()` (ou équivalent) pour **toutes** les I/O
- `poll()` surveille **lecture ET écriture** simultanément
- Fichiers disque ordinaires : exemptés de `poll()`
- Sockets, pipes, FIFOs : **toujours non-bloquants et pilotés par `poll()`**

### 🟡 Fonctionnalités minimum à implémenter

- [ ] Méthodes **GET**, **POST**, **DELETE**
- [ ] Upload de fichiers
- [ ] Site statique complet
- [ ] Pages d'erreur par défaut
- [ ] Fichier de configuration (NGINX-like)
- [ ] Écoute sur plusieurs ports
- [ ] Au moins **1 CGI** (php-CGI ou Python)
- [ ] Codes de statut HTTP précis
- [ ] Gestion des déconnexions client

### 🟢 Bonnes pratiques recommandées

- Tester avec `telnet` et NGINX comme référence
- Stress-tester le serveur (doit rester disponible en permanence)
- Écrire des tests automatisés (Python, Golang, C++)
- Sur macOS : `fcntl()` avec `F_SETFL`, `O_NONBLOCK`, `FD_CLOEXEC` uniquement
- Lire les RFC HTTP avant de commencer
- Comparer les comportements avec NGINX en cas de doute

---

*Document généré depuis le sujet officiel Webserv 42 — Version 24.0*
