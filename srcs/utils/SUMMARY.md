# srcs/utils/ — Résumé & TODO

## Rôle dans le projet

Ce dossier doit centraliser **les fonctions et classes utilitaires transverses** : tout ce qui sert dans plusieurs modules (config, http, handlers, cgi) et qui n'appartient à aucun en particulier.

L'idée SOLID : éviter que chaque module duplique sa propre fonction `toString`, sa propre normalisation de string, sa propre détection MIME, etc. Mais aussi éviter le piège du "fourre-tout God-utils" — chaque utilitaire doit avoir une raison précise d'exister, sinon il appartient à un module métier.

### Catégories candidates pour ce dossier

| Catégorie                | Exemples concrets                                                          |
| ------------------------ | -------------------------------------------------------------------------- |
| String helpers           | `toString(size_t)`, `trim`, `toLower`, `split`, `startsWith`, `endsWith`   |
| Time helpers             | `nowMs()`, `formatHttpDate(time_t)` pour les headers `Date:` / `Last-Modified:` |
| File I/O                 | `readFileToString(path)`, `fileExists`, `isDirectory`, `getFileSize`      |
| MIME types               | `mimeFromExtension(".png")` → `"image/png"`                                |
| HTTP utilities           | `urlDecode(string)`, `getReasonPhrase(int code)` (factorisé depuis ErrorHandler) |
| Logging                  | `Logger::info/warn/error(...)` (optionnel mais propre)                     |

---

## État actuel

**Le dossier est vide.** Seul un `.gitkeep` est présent. Aucun utilitaire centralisé.

Conséquence : des helpers sont **dupliqués ou inline-écrits** un peu partout dans le code :

- `toString(size_t)` est défini :
  - inline dans [ConfigStruct.hpp:11](../../includes/config/ConfigStruct.hpp#L11) (utilisé par config + handlers)
  - en static dans [Lexer.cpp:16](../config/Lexer.cpp#L16) (doublon !)
  - en static dans [ServerHandler.cpp:15](../handlers/ServerHandler.cpp#L15) sous le nom `sizeToStr`
- `addrToStr(uint32_t)` est inline dans [ConfigStruct.hpp:17](../../includes/config/ConfigStruct.hpp#L17) — utilisé par config + handlers.
- `getReason(int code)` est une méthode privée de [ErrorHandler.cpp:9](../http/ErrorHandler.cpp#L9) — devrait être un util public.
- Pas de helper de lecture de fichier : la lecture du `.conf` se fait à la main dans [ConfigLoader.cpp:29-38](../config/ConfigLoader.cpp#L29-L38) avec `std::ifstream` + `rdbuf()`. Le futur `StaticHandler` aura besoin de la même chose.

---

## TODO — points manquants

### 1. ❗ Dédupliquer `toString` / `sizeToStr`
Créer `srcs/utils/StringUtils.cpp` + `includes/utils/StringUtils.hpp` avec :

```cpp
namespace strutil {
    std::string toString(size_t n);
    std::string toString(int n);
    std::string toString(long n);
    std::string toLower(const std::string& s);
    std::string trim(const std::string& s);
    bool        startsWith(const std::string& s, const std::string& prefix);
    std::vector<std::string> split(const std::string& s, char sep);
}
```

Migrer les 3 définitions actuelles vers cette seule source. Garder `addrToStr` à part dans un `NetUtils` ou le laisser dans `ConfigStruct.hpp` (c'est typé réseau).

### 2. ❗ Helper de lecture de fichier disque
Le `StaticHandler` (à écrire — voir [srcs/http/SUMMARY.md TODO 8](../http/SUMMARY.md)) aura besoin de lire un fichier complet pour le servir. Le `ConfigLoader` fait déjà ça à la main. → Factoriser :

```cpp
namespace fileutil {
    bool        exists(const std::string& path);
    bool        isRegularFile(const std::string& path);
    bool        isDirectory(const std::string& path);
    size_t      size(const std::string& path);
    std::string read(const std::string& path);  // throw si échec
    std::vector<std::string> listDir(const std::string& path); // pour autoindex
}
```

Important : tout passe par `stat()`, `opendir`/`readdir`/`closedir`, `open`/`read`/`close` — tous dans la liste autorisée.

### 3. ❗ Table MIME types
`StaticHandler` doit renvoyer le bon `Content-Type` selon l'extension du fichier servi. À écrire une fois pour toutes :

```cpp
namespace mime {
    std::string typeOf(const std::string& path); // "/foo/bar.png" -> "image/png"
}
```

Implementation simple : un `std::map<std::string, std::string>` initialisé une fois avec les extensions courantes (`html`, `htm`, `css`, `js`, `png`, `jpg`, `jpeg`, `gif`, `svg`, `pdf`, `txt`, `json`, `xml`, `mp4`, ...). Fallback : `application/octet-stream`.

### 4. ❗ Factoriser `getReasonPhrase(int code)` hors d'`ErrorHandler`
Aujourd'hui [ErrorHandler.cpp:9-21](../http/ErrorHandler.cpp#L9-L21) contient un switch. Mais le code de status est aussi utilisé dans :
- La réponse de succès (`200 OK`, `204 No Content`, `301 Moved Permanently`, etc.).
- La réponse CGI (mapping Status: header).

→ Sortir dans `srcs/utils/HttpUtils.cpp` :

```cpp
namespace httputil {
    const char* reasonPhrase(int code);  // "404" -> "Not Found"
    std::string urlDecode(const std::string& encoded);  // %20 -> ' '
    std::string formatDate(time_t t);  // "Sun, 06 Nov 1994 08:49:37 GMT"
}
```

`urlDecode` sera nécessaire dès que les URI auront des `%XX` (queries, noms de fichiers avec espaces).

### 5. Helper temps pour timeouts
Le `TimerHandler` (à écrire, voir [srcs/handlers/SUMMARY.md TODO 3](../handlers/SUMMARY.md)) a besoin de `time_t` ou `clock_gettime`. Centraliser :

```cpp
namespace timeutil {
    long long nowMs();          // monotonic, pour les timeouts
    std::string httpDate();     // pour header Date:
}
```

⚠️ `clock_gettime` n'est pas dans la liste autorisée. Donc fallback sur `time(NULL)` (précision 1s, acceptable pour timeout 30s).

### 6. (Optionnel) Petit logger
Aujourd'hui les logs partent en `std::cout` / `std::cerr` éparpillés. Un mini-logger :

```cpp
namespace log {
    void info(const std::string& msg);
    void warn(const std::string& msg);
    void error(const std::string& msg);
}
```

Préfixe `[INFO] [WARN] [ERROR]` + horodatage. Pas nécessaire pour la note mais propre à la défense.

### 7. (Optionnel) Auto-test minimal des utils
Un main alternatif dans `test/` qui appelle chaque util avec des cas connus et compare. Aide à la régression. Le sujet recommande de tester en Python/Go, mais un test C++ direct sur les utils internes est complémentaire.

---

## Priorisation

| Priorité  | Tâche                                                              |
| --------- | ------------------------------------------------------------------ |
| Important | 1 (dédupliquer toString), 2 (file helpers — bloque StaticHandler), 3 (MIME — bloque StaticHandler), 4 (reasonPhrase + urlDecode) |
| Utile     | 5 (time helpers — utile pour timeout), 6 (logger), 7 (tests)       |

**État global :** dossier 0% rempli, mais ce n'est pas un blocker en soi — c'est un **refactor de qualité**. Les helpers existent déjà ailleurs, juste pas au bon endroit. Les points 2, 3 et 4 deviennent critiques **dès qu'on attaque sérieusement `StaticHandler`** : autant les écrire à ce moment-là plutôt que ré-inliner du code.

Recommandation : ne pas créer les fichiers maintenant à vide, mais les créer **au fil des besoins**, juste avant chaque migration de helper. Le but est d'avoir tout dans `utils/` à la fin du projet, pas d'avoir un dossier rempli de stubs.
