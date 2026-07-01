#!/bin/bash

HOST="localhost"
PORT="7001"
BASE="http://$HOST:$PORT"

GREEN="\033[0;32m"
RED="\033[0;31m"
YELLOW="\033[1;33m"
NC="\033[0m"
BOLD="\033[1m"

PASS=0
FAIL=0

check() {
    local desc="$1"
    local actual="${@: -1}"
    local matched=0

    for expected in "${@:2:$#-2}"; do
        [ "$actual" = "$expected" ] && matched=1
    done

    if [ $matched -eq 1 ]; then
        echo -e "${GREEN}[PASS]${NC} $desc"
        PASS=$((PASS + 1))
    else
        echo -e "${RED}[FAIL]${NC} $desc — attendu ${BOLD}${*:2:$#-2}${NC}, recu ${BOLD}$actual${NC}"
        FAIL=$((FAIL + 1))
    fi
}

section() {
    echo ""
    echo -e "${YELLOW}=== $1 ===${NC}"
}

# ─── GET ──────────────────────────────────────────────────────────────────────

section "GET basique"

code=$(curl -s -o /dev/null -w "%{http_code}" "$BASE/")
check "GET / -> 200" "200" "$code"

code=$(curl -s -o /dev/null -w "%{http_code}" "$BASE/nope_inexistant")
check "GET fichier inexistant -> 404" "404" "$code"

code=$(curl -s -o /dev/null -w "%{http_code}" "$BASE/upload")
check "GET sur /upload (method non autorisee) -> 405" "405" "$code"

# ─── UPLOAD MULTIPART ─────────────────────────────────────────────────────────

section "POST multipart upload"

echo "hello webserv" > /tmp/test_upload.txt

code=$(curl -s -o /dev/null -w "%{http_code}" \
    -X POST "$BASE/upload" \
    -F "file=@/tmp/test_upload.txt")
check "POST fichier texte -> 200/201" "200" "201" "$code"

echo "contenu binaire factice" > /tmp/test.bin
code=$(curl -s -o /dev/null -w "%{http_code}" \
    -X POST "$BASE/upload" \
    -F "file=@/tmp/test.bin;type=application/octet-stream")
check "POST fichier binaire -> 200/201" "200" "201" "$code"

python3 -c "print('A' * 100)" > /tmp/small.txt
code=$(curl -s -o /dev/null -w "%{http_code}" \
    -X POST "$BASE/upload" \
    -F "file=@/tmp/small.txt" \
    -F "champ=valeur")
check "POST avec champ texte + fichier -> 200/201" "200" "201" "$code"

code=$(curl -s -o /dev/null -w "%{http_code}" \
    -X POST "$BASE/upload" \
    -H "Content-Type: multipart/form-data; boundary=----boundary123" \
    --data-raw $'------boundary123\r\nContent-Disposition: form-data; name="file"; filename="a.txt"\r\nContent-Type: text/plain\r\n\r\nhello\r\n------boundary123--')
check "POST multipart brut -> 200/201" "200" "201" "$code"

code=$(curl -s -o /dev/null -w "%{http_code}" \
    -X POST "$BASE/upload" \
    -H "Content-Type: text/plain" \
    --data "pas multipart du tout")
check "POST sans multipart sur /upload -> 200 (body classique)" "200" "$code"

# ─── BODY TROP GRAND ─────────────────────────────────────────────────────────

section "client_max_body_size"

python3 -c "import sys; sys.stdout.buffer.write(b'X' * 2 * 1024 * 1024)" > /tmp/big.bin
code=$(curl -s -o /dev/null -w "%{http_code}" \
    -X POST "http://$HOST:8084/" \
    -F "file=@/tmp/big.bin")
check "POST > 1m sur port 8084 -> 413" "413" "$code"

# ─── DELETE ───────────────────────────────────────────────────────────────────

section "DELETE"

echo "a supprimer" > /tmp/todelete.txt
curl -s -o /dev/null -X POST "$BASE/upload" -F "file=@/tmp/todelete.txt" > /dev/null

code=$(curl -s -o /dev/null -w "%{http_code}" \
    -X DELETE "$BASE/upload/todelete.txt")
check "DELETE fichier existant -> 200 ou 204" "204" "200" "$code"

code=$(curl -s -o /dev/null -w "%{http_code}" \
    -X DELETE "$BASE/upload/fichier_qui_nexiste_pas.txt")
check "DELETE fichier inexistant -> 404" "404" "$code"

code=$(curl -s -o /dev/null -w "%{http_code}" \
    -X DELETE "$BASE/")
check "DELETE sur location GET-only -> 405" "405" "$code"

# ─── METHODES NON IMPLEMENTEES ────────────────────────────────────────────────

section "Methodes non implementees"

code=$(curl -s -o /dev/null -w "%{http_code}" \
    -X PUT "$BASE/" \
    --data "test")
check "PUT -> 501" "501" "$code"

code=$(curl -s -o /dev/null -w "%{http_code}" \
    -X PATCH "$BASE/" \
    --data "test")
check "PATCH -> 501" "501" "$code"

# ─── AUTOINDEX ────────────────────────────────────────────────────────────────

section "Autoindex"

code=$(curl -s -o /dev/null -w "%{http_code}" "$BASE/api/bli/")
check "GET /api/bli/ (autoindex on) -> 200" "200" "$code"

body=$(curl -s "$BASE/api/bli/")
echo "$body" | grep -qi "href"
if [ $? -eq 0 ]; then
    echo -e "${GREEN}[PASS]${NC} Autoindex contient des liens href"
    PASS=$((PASS + 1))
else
    echo -e "${RED}[FAIL]${NC} Autoindex ne contient pas de liens href"
    FAIL=$((FAIL + 1))
fi

# ─── RETURN / REDIRECT ────────────────────────────────────────────────────────

section "Return / redirect"

code=$(curl -s -o /dev/null -w "%{http_code}" "$BASE/old-page")
check "GET /old-page (return 500) -> 500" "500" "$code"

code=$(curl -s -o /dev/null -w "%{http_code}" "http://$HOST:8084/")
check "GET / sur port 8084 (return 501) -> 501" "501" "$code"

# ─── KEEP-ALIVE / PLUSIEURS REQUETES ─────────────────────────────────────────

section "Connexions multiples"

for i in 1 2 3 4 5; do
    code=$(curl -s -o /dev/null -w "%{http_code}" "$BASE/")
    if [ "$code" != "200" ]; then
        echo -e "${RED}[FAIL]${NC} Requete $i -> $code au lieu de 200"
        FAIL=$((FAIL + 1))
    fi
done
echo -e "${GREEN}[PASS]${NC} 5 GET consecutifs -> 200"
PASS=$((PASS + 1))

# Requetes paralleles
for i in $(seq 1 10); do
    curl -s -o /dev/null "$BASE/" &
done
wait
echo -e "${GREEN}[INFO]${NC} 10 requetes paralleles envoyees (verifier que le serveur tient)"

# ─── HEADERS SPECIAUX ────────────────────────────────────────────────────────

section "Headers"

code=$(curl -s -o /dev/null -w "%{http_code}" \
    -H "Host: test" "$BASE/")
check "GET avec Host header -> 200" "200" "$code"

code=$(curl -s -o /dev/null -w "%{http_code}" \
    "$BASE/" \
    -H "Content-Length: 0")
check "GET avec Content-Length: 0 -> 200" "200" "$code"

# ─── RESUME ──────────────────────────────────────────────────────────────────

echo ""
echo -e "${YELLOW}================================${NC}"
echo -e "  PASS: ${GREEN}$PASS${NC}  |  FAIL: ${RED}$FAIL${NC}"
echo -e "${YELLOW}================================${NC}"

rm -f /tmp/test_upload.txt /tmp/test.bin /tmp/small.txt /tmp/big.bin /tmp/todelete.txt

[ $FAIL -eq 0 ] && exit 0 || exit 1
