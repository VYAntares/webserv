#!/usr/bin/env bash
# ============================================================================
# BONUS — validation "handle multiple CGI types"
#
# demo.conf déclare deux interpréteurs dans /cgi-bin :
#     cgi_pass .py /usr/bin/python3
#     cgi_pass .sh /usr/bin/bash
#
# Prérequis : ./webserv conf/demo.conf
# Usage     : bash tests/bonus_multi_cgi.sh [host:port]
# ============================================================================
HOST="${1:-localhost:8080}"
BASE="http://$HOST"
PASS=0; FAIL=0

ok() { PASS=$((PASS+1)); printf '  \033[32mPASS\033[0m %s\n' "$1"; }
ko() { FAIL=$((FAIL+1)); printf '  \033[31mFAIL\033[0m %s\n' "$1"; }

echo "=== BONUS multi-CGI — $BASE ==="
echo

echo "[1] CGI Python : GET avec query string"
R=$(curl -s -w '\n%{http_code}' "$BASE/cgi-bin/hello.py?type=python")
CODE=$(echo "$R" | tail -1)
if [ "$CODE" = "200" ] && echo "$R" | grep -q "Hello from CGI"; then
	ok "hello.py -> 200 + sortie correcte"
else
	ko "hello.py -> $CODE"
fi

echo "[2] CGI Python : POST (body lu jusqu'à EOF)"
R=$(curl -s -X POST --data "abcdefghij" "$BASE/cgi-bin/post_echo.py")
if echo "$R" | grep -q '"body_bytes_read": 10'; then
	ok "post_echo.py a lu 10 octets"
else
	ko "post_echo.py : $R"
fi

echo "[3] CGI Bash : GET avec query string"
R=$(curl -s -w '\n%{http_code}' "$BASE/cgi-bin/calc.sh?a=6&b=7")
CODE=$(echo "$R" | tail -1)
if [ "$CODE" = "200" ] && echo "$R" | grep -q '"sum": *13'; then
	ok "calc.sh -> 200, 6+7=13 calculé par bash"
else
	ko "calc.sh -> $CODE : $(echo "$R" | head -1)"
fi

echo "[4] CGI Bash : POST"
R=$(curl -s -X POST --data "hello-bash" "$BASE/cgi-bin/calc.sh")
if echo "$R" | grep -q '"body_bytes_read": *10'; then
	ok "calc.sh a lu le body en POST"
else
	ko "calc.sh POST : $R"
fi

echo "[5] Les deux types cohabitent dans la même location"
P=$(curl -s -o /dev/null -w '%{http_code}' "$BASE/cgi-bin/env.py")
S=$(curl -s -o /dev/null -w '%{http_code}' "$BASE/cgi-bin/calc.sh")
if [ "$P" = "200" ] && [ "$S" = "200" ]; then
	ok ".py (python3) et .sh (bash) exécutés selon l'extension"
else
	ko ".py=$P .sh=$S"
fi

echo
echo "=== Résultat : $PASS PASS, $FAIL FAIL ==="
[ "$FAIL" -eq 0 ]
