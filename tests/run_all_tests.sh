#!/usr/bin/env bash
# ============================================================================
# Suite de tests partie obligatoire (curl + python3 pour le bas niveau).
# Complète les pages navigateur de www/default/ avec ce qu'un navigateur ne
# sait pas faire : requêtes malformées (400), chunked, headers géants...
#
# Prérequis : ./webserv conf/demo.conf
# Usage     : bash tests/run_all_tests.sh [host:port]
# ============================================================================
HOST="${1:-localhost:8080}"
BASE="http://$HOST"
HOST2="${HOST%:*}:8081"
PASS=0; FAIL=0

ok() { PASS=$((PASS+1)); printf '  \033[32mPASS\033[0m %s\n' "$1"; }
ko() { FAIL=$((FAIL+1)); printf '  \033[31mFAIL\033[0m %s\n' "$1"; }

# check <description> <code_attendu (regex, ex "201|200")> <args curl...>
check() {
	local desc="$1" want="$2"; shift 2
	local got
	got=$(curl -s -o /dev/null -w '%{http_code}' --max-time 10 "$@")
	if echo "$got" | grep -qE "^($want)$"; then
		ok "$desc ($got)"
	else
		ko "$desc (attendu $want, reçu $got)"
	fi
}

echo "=== Suite obligatoire — $BASE ==="
echo
echo "--- GET / statique ---"
check "GET index"                      200 "$BASE/default/index.html"
check "GET index implicite (/default/)" 200 "$BASE/default/"
check "GET fichier texte"              200 "$BASE/files/hello.txt"
check "GET inexistant -> 404"          404 "$BASE/nexiste-pas"
check "GET autoindex /files/"          200 "$BASE/files/"
check "GET /files sans slash -> 301"   301 "$BASE/files"
check "GET /private/ -> 403 (listing)" 403 "$BASE/private/"
check "GET /old-page -> 301"           301 "$BASE/old-page"
check "GET /redirect302 -> 302"        302 "$BASE/redirect302"

echo
echo "--- HEAD ---"
check "HEAD /files/hello.txt -> 200"   200 -I "$BASE/files/hello.txt"
BODY=$(curl -s -X HEAD --max-time 5 "$BASE/files/hello.txt" 2>/dev/null)
[ -z "$BODY" ] && ok "HEAD sans body" || ko "HEAD a renvoyé un body"

echo
echo "--- Méthodes refusées ---"
check "POST sur route GET-only -> 405"   405 -X POST --data x "$BASE/default/index.html"
check "DELETE sur route GET-only -> 405" 405 -X DELETE "$BASE/default/index.html"
check "PATCH -> 501"                     501 -X PATCH --data x "$BASE/default/index.html"

echo
echo "--- POST / upload / DELETE ---"
check "POST brut 1 Ko -> 201/200"      "201|200" -X POST --data-binary "$(head -c 1024 /dev/zero | tr '\0' 'A')" "$BASE/upload/cli_raw.txt"
check "GET du fichier posté"           200 "$BASE/upload/cli_raw.txt"
check "DELETE du fichier -> 204"       204 -X DELETE "$BASE/upload/cli_raw.txt"
check "GET après DELETE -> 404"        404 "$BASE/upload/cli_raw.txt"
check "DELETE inexistant -> 404"       404 -X DELETE "$BASE/upload/jamais_vu.txt"

TMPF=$(mktemp); head -c $((2*1024*1024)) /dev/urandom > "$TMPF"
check "Upload multipart 2 Mo -> 201/200" "201|200" -F "file=@$TMPF;filename=cli_mp.bin" "$BASE/upload"
SIZE=$(curl -s --max-time 10 "$BASE/upload/cli_mp.bin" | wc -c)
[ "$SIZE" = "2097152" ] && ok "fichier multipart intact (2097152 octets)" || ko "taille relue $SIZE"
check "DELETE multipart -> 204"        204 -X DELETE "$BASE/upload/cli_mp.bin"
rm -f "$TMPF"

echo
echo "--- Limites de body ---"
check "POST 50 o sur /tiny -> 201/200" "201|200" -X POST --data-binary "$(head -c 50 /dev/zero | tr '\0' 'B')" "$BASE/tiny/cli_tiny.txt"
curl -s -X DELETE "$BASE/upload/cli_tiny.txt" > /dev/null
check "POST 200 o sur /tiny -> 413"    413 -X POST --data-binary "$(head -c 200 /dev/zero | tr '\0' 'B')" "$BASE/tiny/cli_tiny.txt"

echo
echo "--- CGI ---"
check "CGI GET -> 200"                 200 "$BASE/cgi-bin/hello.py?x=1"
check "CGI Status: 404 honoré"         404 "$BASE/cgi-bin/test_status.py"
check "CGI exit(1) -> 500"             500 "$BASE/cgi-bin/crash.py"
check "CGI SIGSEGV -> 502"             502 "$BASE/cgi-bin/sigsegv.py"

R=$(curl -s -X POST --data "1234567890" "$BASE/cgi-bin/post_echo.py")
echo "$R" | grep -q '"body_bytes_read": 10' \
	&& ok "CGI POST : 10 octets lus" || ko "CGI POST : $R"

echo
echo "--- Chunked (le serveur doit dé-chunker pour le CGI) ---"
R=$(curl -s -H "Transfer-Encoding: chunked" -H "Content-Length:" \
	--data-binary "$(head -c 100000 /dev/zero | tr '\0' 'C')" "$BASE/cgi-bin/post_echo.py")
echo "$R" | grep -q '"body_bytes_read": 100000' \
	&& ok "chunked 100 Ko dé-chunké, CGI a lu 100000 octets" \
	|| ko "chunked : $(echo "$R" | grep body_bytes_read)"

check "POST chunked brut -> fichier créé" "201|200" -X POST -H "Transfer-Encoding: chunked" \
	--data-binary "corps-en-chunked" "$BASE/upload/cli_chunked.txt"
curl -s -X DELETE "$BASE/upload/cli_chunked.txt" > /dev/null

echo
echo "--- Requêtes malformées (python, socket brut) ---"
python3 - "$HOST" <<'EOF'
import socket, sys, select, time
host, port = sys.argv[1].split(":")
def raw(data, timeout=5):
    # envoie en lisant en parallele : le serveur peut repondre (ex: 431) et
    # fermer avant d'avoir tout recu — il faut capter la reponse avant le RST
    s = socket.create_connection((host, int(port)), timeout=timeout)
    s.setblocking(False)
    out, sent = b"", 0
    deadline = time.time() + timeout
    while time.time() < deadline:
        want_write = [s] if sent < len(data) else []
        r, w, _ = select.select([s], want_write, [], 0.1)
        if r:
            try:
                chunk = s.recv(4096)
            except OSError:
                break
            if not chunk:
                break
            out += chunk
            if b"\r\n\r\n" in out:
                break
        if w:
            try:
                sent += s.send(data[sent:sent + 8192])
            except OSError:
                sent = len(data)    # serveur deja ferme : on arrete d'ecrire
        if not r and not w and sent >= len(data) and out:
            break
    s.close()
    return out.decode("latin-1", "replace")

def status(resp):
    line = resp.split("\r\n", 1)[0]
    parts = line.split(" ")
    return parts[1] if len(parts) > 1 else "???"

tests = [
    ("premiere ligne invalide -> 400", b"N'IMPORTE QUOI\r\n\r\n", "400"),
    ("methode minuscule -> 400", b"get / HTTP/1.1\r\nHost: x\r\n\r\n", "400"),
    ("mauvaise version HTTP -> 400", b"GET / HTTP/9.9\r\nHost: x\r\n\r\n", "400"),
    ("header sans ': ' -> 400", b"GET / HTTP/1.1\r\nHost x\r\n\r\n", "400"),
    ("Content-Length non numerique -> 400", b"POST /upload/x HTTP/1.1\r\nHost: x\r\nContent-Length: abc\r\n\r\n", "400"),
    ("headers > 32 Ko -> 431", b"GET / HTTP/1.1\r\nHost: x\r\nX-Big: " + b"A" * 40000 + b"\r\n\r\n", "431"),
]
for name, payload, want in tests:
    try:
        got = status(raw(payload))
    except Exception as e:
        got = "EXC:" + str(e)
    mark = "\033[32mPASS\033[0m" if got == want else "\033[31mFAIL\033[0m"
    print("  %s %s (recu %s)" % (mark, name, got))

# keep-alive : 2 requetes successives sur la meme connexion TCP
import http.client
try:
    conn = http.client.HTTPConnection(host, int(port), timeout=5)
    conn.request("GET", "/files/hello.txt")
    r1 = conn.getresponse(); r1.read()
    conn.request("GET", "/files/hello.txt")   # reutilise la connexion
    r2 = conn.getresponse(); r2.read()
    conn.close()
    okka = (r1.status == 200 and r2.status == 200)
except Exception as e:
    okka = False
mark = "\033[32mPASS\033[0m" if okka else "\033[31mFAIL\033[0m"
print("  %s keep-alive : 2 requetes servies sur 1 connexion" % mark)
EOF

echo
echo "--- Multi-ports (demo.conf) ---"
check "port 8080 repond"               200 "$BASE/default/index.html"
check "port 8081 repond"               200 "http://$HOST2/"
C1=$(curl -s --max-time 5 "$BASE/" | head -c 200)
C2=$(curl -s --max-time 5 "http://$HOST2/" | head -c 200)
[ "$C1" != "$C2" ] && ok "contenus différents sur 8080 et 8081" || ko "même contenu sur les 2 ports"

echo
echo "=== Résultat curl : $PASS PASS, $FAIL FAIL (les tests python s'affichent au-dessus) ==="
[ "$FAIL" -eq 0 ]
