# Tests webserv

Lancer le serveur d'abord :

```sh
make && ./webserv conf/demo.conf
```

`conf/demo.conf` est un sur-ensemble de `conf/default.conf` : mêmes routes, plus
`/tiny` (413), `/private` (403), `/redirect302`, le CGI shell (`.sh`), les pages
d'erreur personnalisées et un second serveur sur le port 8081.

## Dans le navigateur

Ouvrir <http://localhost:8080/default/> : chaque page (GET/HEAD, POST, upload,
DELETE, CGI, erreurs, stress, bonus) contient des tests automatiques « Tout
lancer » + des démos manuelles (formulaires, uploads avec progression...).

## En ligne de commande

```sh
bash tests/run_all_tests.sh            # partie obligatoire : méthodes, codes,
                                       # chunked, requêtes malformées, keep-alive,
                                       # multi-ports (curl + python3)
bash tests/bonus_cookies_sessions.sh   # bonus : cookies + sessions serveur
bash tests/bonus_multi_cgi.sh          # bonus : CGI Python (.py) + Bash (.sh)
python3 tests/stress_test.py           # stress : 500 requêtes / 50 threads
```

Tous acceptent `host:port` en premier argument (défaut `localhost:8080`).

## État de session côté serveur

`session.py` stocke les sessions dans `www/cgi-bin/sessions/<sid>.json`
(nettoyées après 1 h). Le client est identifié par le cookie `sid`.

## Limites connues (constatées, code non modifié)

1. `CGIReadHandler::parseCgi()` ne relaie que `Status:` et `Content-Type:` —
   le `Set-Cookie:` émis par un CGI n'atteint pas le client (test en WARN dans
   `bonus_cookies_sessions.sh` et sur `/default/bonus.html`). La page bonus
   contourne en posant le cookie côté JavaScript ; le reste du flux
   (HTTP_COOKIE → CGI, état serveur, isolation, logout) est validé.

2. CGI plus lent que le timeout (30 s) : la requête ne pend pas indéfiniment ✔,
   mais le client reçoit une réponse vide au lieu du 504 prévu —
   `EventLoop::checkTimeOut()` collecte le ClientHandler et le CGIReadHandler
   dans le même balayage, donc le 504 envoyé par `~CGIReadHandler()` n'est
   jamais écrit sur une connexion déjà détruite. Test manuel sur
   `/default/cgi.html` (bouton timeout).
