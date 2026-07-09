# AJOUTS_SUJET.md — Ce qui a été ajouté pour le bon fonctionnement

Après correction des bugs, j'ai relu le sujet (`webserv_subject.md`) point par point pour
vérifier ce qui était **fonctionnellement présent** et ce qui **manquait pour pouvoir
démontrer** le projet en évaluation. Le code couvrait déjà toutes les fonctionnalités
**mandatory** ; ce qui manquait, c'était le **matériel de test** exigé par le sujet.

> Citation du sujet (IV.3) :
> *« You must provide configuration files and default files to test and demonstrate that
> every feature works during the evaluation. »*

Le dépôt ne contenait aucun dossier `www/` ni fichier servi, et la config
`conf/default.conf` pointe vers des chemins absolus (`/var/www/...`) qui n'existent pas sur
une machine d'évaluation. Impossible en l'état de montrer que le serveur fonctionne.

---

## 1. `conf/demo.conf` — configuration portable et complète

Une configuration **auto-suffisante** utilisant des chemins **relatifs** au dépôt
(`root www`), donc utilisable immédiatement après `git clone` + `make` :

```
./webserv conf/demo.conf
```

Elle exerce **toutes** les fonctionnalités demandées par le sujet, une location par
fonctionnalité :

| Location | Fonctionnalité du sujet démontrée |
|----------|-----------------------------------|
| `location /` | site statique, `index`, `allowed_methods` (GET seul) |
| `location /files` | **directory listing** (`autoindex on`) |
| `location /upload` | **upload de fichiers** (`upload_store`), POST/DELETE/GET |
| `location /cgi-bin` | **exécution CGI** basée sur l'extension (`.py`) |
| `location /old-page` | **redirection HTTP** (`return 301`) |
| `client_max_body_size 10m` | **taille max du body** |
| 2ᵉ `server` (port 8084, dans default.conf) | **écoute multi-ports** |

Aucune modification du comportement du serveur : ce n'est qu'un fichier de configuration.

## 2. `www/` — fichiers servis pour la démonstration

Arborescence minimale nécessaire pour que chaque location renvoie quelque chose de visible :

```
www/
├── index.html            → page d'accueil (liens vers chaque fonctionnalité + formulaire d'upload)
├── files/
│   └── hello.txt         → fichier listé/servi par l'autoindex de /files
├── cgi-bin/
│   └── hello.py          → script CGI de démonstration
└── upload/
    └── .gitkeep          → dossier de dépôt des uploads (versionné vide)
```

### `www/index.html`
Page d'accueil listant les fonctionnalités et contenant un **formulaire d'upload
multipart** (`enctype="multipart/form-data"`) pour tester l'upload **depuis un navigateur**,
comme l'exige le sujet (*« Your server must be compatible with standard web browsers »*).

### `www/cgi-bin/hello.py`
Script CGI conforme à CGI/1.1 : il lit **stdin** (body du POST), imprime un bloc d'en-têtes
CGI (`Content-Type`) puis du HTML affichant `REQUEST_METHOD`, `QUERY_STRING` et le body reçu.
Il prouve concrètement que :
- les variables d'environnement CGI sont bien transmises (`QUERY_STRING`, `REQUEST_METHOD`…),
- le body de la requête est bien envoyé sur le **stdin** du script,
- la sortie du script est correctement transformée en réponse HTTP.

> Note : le sujet demande *« at least one CGI »*. Ici Python 3 (`/usr/bin/python3`). Si
> l'interpréteur est ailleurs sur la machine d'évaluation, ajuster la ligne `cgi_pass` de
> `conf/demo.conf`.

### `www/upload/`
Dossier cible des uploads, déclaré via `upload_store ./www/upload`. Versionné vide grâce à
`.gitkeep` pour que la structure existe après un clone.

---

## Fonctionnalités du sujet : état après ce passage

Toutes **déjà implémentées dans le code** (corrigées si besoin, cf. `CORRECTIONS_BUGS.md`),
et désormais **démontrables** grâce aux ajouts ci-dessus :

- [x] Fichier de configuration en argument, chemin par défaut sinon
- [x] Serveur non bloquant, **un seul `epoll`** pour toutes les I/O (lecture + écriture)
- [x] Écoute multi-ports / multi-serveurs
- [x] Méthodes **GET, POST, DELETE**
- [x] Site **statique**
- [x] **Upload** de fichiers (multipart + POST brut vers `upload_store`)
- [x] **CGI** par extension, body sur stdin, EOF = fin de sortie, un-chunk avant CGI
- [x] **Pages d'erreur par défaut** (générées si aucune n'est configurée) + `error_page` custom
- [x] `client_max_body_size`
- [x] Directives de route : `allowed_methods`, `return` (redirection), `root`, `autoindex`,
      `index`, `upload_store`, `cgi_pass`
- [x] **Résilience** : le serveur ne crashe sous aucune des entrées testées (malformées,
      partielles, corps trop gros, CGI qui boucle) et aucune requête ne pend indéfiniment
      (timeout client **et** timeout CGI)
- [x] Codes de statut HTTP exacts, compatible navigateurs

Le `README.md` requis par le chapitre V existe déjà à la racine.

---

## Comment rejouer la démonstration

```bash
make
./webserv conf/demo.conf
# puis, dans un navigateur ou avec curl :
#   http://127.0.0.1:8080/                     → page d'accueil (index)
#   http://127.0.0.1:8080/files/               → directory listing
#   http://127.0.0.1:8080/cgi-bin/hello.py?a=1 → CGI
#   http://127.0.0.1:8080/old-page             → redirection 301
#   formulaire d'upload sur la page d'accueil  → écrit dans www/upload/
```
