# srcs/config/ — Résumé & TODO

## Rôle dans le projet

Ce dossier transforme **un fichier `.conf` texte** en **une structure `Config` typée** que le reste du serveur consomme. Il est exécuté **une seule fois au démarrage** (dans `main.cpp` via `ConfigLoader`) ; aucune relecture à chaud n'est prévue.

Le sujet exige que la config supporte :

- Plusieurs paires `interface:port` (listen) — un binaire peut écouter sur plusieurs sockets.
- Plusieurs blocs `server` (un par "site").
- Plusieurs blocs `location` par server (route matching par préfixe d'URI, **sans regex**).
- Pages d'erreur par défaut surchargeables (`error_page`).
- Taille max du body client (`client_max_body_size`, avec unités `k`/`m`/`g`).
- Méthodes autorisées par route (`allowed_methods`, parmi GET/POST/DELETE).
- Redirections (`return code [url]`).
- Mapping URL → racine disque (`root`).
- Activation/désactivation du listing de dossier (`autoindex on|off`).
- Fichier par défaut quand l'URI cible un dossier (`index`).
- Dossier d'upload (`upload_store`).
- CGI par extension (`cgi_pass .ext /chemin/interpreteur`).

### Découpage SOLID

Le pipeline suit une chaîne lexer → parser → validator, chaque étape avec une seule responsabilité :

| Classe         | Responsabilité unique (SRP)                                                              |
| -------------- | ---------------------------------------------------------------------------------------- |
| `Lexer`        | Texte brut → flux de `Token` (WORD, LBRACE, RBRACE, SEMI, EOF). Pas de sémantique.       |
| `Parser`       | Flux de tokens → arborescence `Config { Server { Location { ... } } }`. Erreurs syntaxiques. |
| `Validator`    | Cohérence sémantique : doublons cross-server, héritage server→location, valeurs invalides. |
| `ConfigLoader` | Orchestrateur : lit le fichier, enchaîne les trois étapes, expose le `Config` final.     |

OCP : ajouter une directive = ajouter un `else if` dans `Parser::putDirective()` ou `parseDirective()`, sans toucher au reste. DIP : le reste du serveur ne dépend que de `ConfigStruct.hpp` (POD), pas du parser concret.

---

## État actuel

### Lexer ([Lexer.cpp](Lexer.cpp))
**Fait, propre.**
- Tokenize correctement `{`, `}`, `;`, mots, et strings entre quotes (`"..."` ou `'...'`).
- Suit les `line`/`col` pour les messages d'erreur.
- Saute whitespaces et commentaires `#`.
- Gère le cas de la quote non fermée → exception claire.

### Parser ([Parser.cpp](Parser.cpp))
**Largement fait.** Couvre toutes les directives listées dans le sujet :
- `parseServerBlock()` + `parseLocationBlock()` : structure imbriquée OK, détecte les blocs non fermés.
- `parseDirective(Server&)` et `parseDirective(Location&)` : surcharge selon le contexte, rejette les directives mal placées (ex: `upload_store` dans un server, `listen` dans une location).
- `putDirective(BaseBlock&)` : directives communes (root, index, error_page, return, autoindex, client_max_body_size) avec détection de doublons.
- `parseListen()` : split host/port intelligent, valide le port, résout le host via `getaddrinfo()`, gère `*` comme alias `0.0.0.0`, détecte les doublons intra-server.
- `parseClientBody()` : conversion d'unités `k`/`m`/`g` → octets.
- `parseErrorPage()` : codes 400–599, plusieurs codes pour un même path.

### Validator ([Validator.cpp](Validator.cpp))
**Largement fait** (a été rebossé récemment, cf. commit `43f3f5d "validator.cpp"`).
- Rejette config vide (`_c.cfg.empty()`).
- Rejette server sans `listen` ou sans `location`.
- Détecte doublons `listen` cross-server (deux servers qui écoutent sur la même paire addr:port).
- Propage `root`, `index`, `max_body_client`, `autoindex`, `return_path`, `error_page` du server vers ses locations si elles n'ont pas la valeur.
- Vérifie chaque location : `root` ou `return` présent, méthodes valides (GET/POST/DELETE) sans doublons, chemins `error_page` absolus, `upload_store` ⇒ POST autorisé, `cgi_pass` extension commençant par `.` et interpréteur en chemin absolu.

### ConfigLoader ([ConfigLoader.cpp](ConfigLoader.cpp))
**Fait.** Vérifie l'extension `.conf`, `stat()` que c'est un fichier régulier, ouvre, lit le contenu, lance les trois phases dans un try/catch qui wrap les exceptions avec le nom du fichier.

### Structure `Config` ([ConfigStruct.hpp](../../includes/config/ConfigStruct.hpp))
**Fait, propre.** Héritage `Location : BaseBlock`, `Server : BaseBlock` pour mutualiser les champs communs. `addrport = pair<uint32_t, int>`. Utilitaires `toString` et `addrToStr` inline.

---

## TODO — points manquants ou à durcir

### 1. Path de config par défaut
Le sujet dit : *"Your program must use a configuration file, provided as an argument on the command line, or available in a default path."* Actuellement [ConfigLoader.cpp:15-16](ConfigLoader.cpp#L15-L16) rejette si `argc != 2`. → Si aucun argument, tomber sur un chemin par défaut (ex: `./conf/default.conf`) avant d'erreur.

### 2. Validator — points encore manquants (cf. `ideas.txt` racine)

Le fichier `ideas.txt` à la racine liste plusieurs checks dont certains sont déjà faits mais d'autres restent. À auditer :

- **return + autres directives actives** (optionnel) : nginx warne quand `return` est défini avec `root`/`index`/`cgi_pass` (inutile car `return` court-circuite). Pas une erreur fatale, juste un warning informatif.
- **Index sans root** : si `index` est défini mais `root` est vide (après propagation), c'est un cas qui glisse aujourd'hui. À vérifier.
- **Path location malformé** : `parseLocationBlock` accepte n'importe quel mot comme `l.path`. Imposer que ça commence par `/` est cohérent avec nginx.
- **Cohérence `error_page` codes** : codes entre 400 et 599 OK côté parser ; rien ne vérifie que le fichier pointé existe au filesystem. Sujet n'exige pas, mais ça serait robuste.

### 3. Gestion d'erreur de `getaddrinfo`
[Parser.cpp:429](Parser.cpp#L429) : si `getaddrinfo` échoue, on throw, mais on ne **freeaddrinfo** pas `res` (qui pourrait être partiellement alloué selon les implémentations). À sécuriser : freeaddrinfo dans tous les chemins.

### 4. `parseListen` n'autorise pas IPv6
Pas exigé par le sujet, mais `hints.ai_family = AF_INET` ferme la porte. À garder comme limitation documentée.

### 5. Comportement quand `autoindex` non défini et URI est un dossier
La struct utilise `autoindex = -1` comme "non défini". Après propagation, si toujours -1 → comportement par défaut à décider : nginx renvoie 403 dans ce cas. Cette décision est plutôt côté `StaticHandler`, mais il faut s'assurer que la valeur `-1` finale est gérable.

### 6. Tests unitaires de la config
Le sujet recommande de tester en Python/Go. Il manque :
- Config minimale valide (1 server, 1 location, 1 listen) → passe.
- Config sans server → rejette.
- Config avec deux servers même `listen` → rejette.
- Config avec directive inconnue → rejette avec message clair.
- Config avec quote non fermée, accolade non fermée, point-virgule manquant → messages d'erreur clairs avec `line:col`.

### 7. `printConfig()` déclaré mais pas implémenté
[ConfigLoader.hpp:14](../../includes/config/ConfigLoader.hpp#L14) déclare `printConfig() const` mais aucune définition dans le .cpp. Soit l'implémenter (debug utile), soit retirer la déclaration pour éviter le link error potentiel quand quelqu'un l'appellera.

### 8. Nom de méthode `propogateServerToLocations` → typo
`propogate` → `propagate`. Petit détail, mais le code est public à la défense.

---

## Priorisation

| Priorité  | Tâche                                                                |
| --------- | -------------------------------------------------------------------- |
| Critique  | 1 (chemin par défaut), 7 (`printConfig` à clarifier)                 |
| Important | 2 (validations résiduelles), 6 (tests)                               |
| Utile     | 3 (freeaddrinfo robuste), 5 (autoindex défaut), 8 (typo)             |

Le pipeline config est globalement la partie **la plus mature** du projet. Aucun gros chantier ; surtout du polissage et l'ajout du chemin par défaut.
