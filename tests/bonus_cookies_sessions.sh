#!/usr/bin/env bash
# ============================================================================
# BONUS — validation cookies & sessions
#
# Prérequis : ./webserv conf/demo.conf   (ou default.conf, routes identiques)
# Usage     : bash tests/bonus_cookies_sessions.sh [host:port]
# ============================================================================
HOST="${1:-localhost:8080}"
BASE="http://$HOST"
PASS=0; FAIL=0; WARN=0

ok()   { PASS=$((PASS+1)); printf '  \033[32mPASS\033[0m %s\n' "$1"; }
ko()   { FAIL=$((FAIL+1)); printf '  \033[31mFAIL\033[0m %s\n' "$1"; }
warn() { WARN=$((WARN+1)); printf '  \033[33mWARN\033[0m %s\n' "$1"; }

json_get() { # json_get <json> <clé> : extrait "clé": valeur (string ou nombre)
	echo "$1" | grep -o "\"$2\": *\"[^\"]*\"" | head -1 | sed 's/.*: *"\(.*\)"/\1/' \
	|| true
}
json_num() {
	echo "$1" | grep -o "\"$2\": *[0-9]*" | head -1 | grep -o '[0-9]*$'
}

echo "=== BONUS cookies & sessions — $BASE ==="
echo

# ---------------------------------------------------------------- 1. cookie -> CGI
echo "[1] Le cookie du client est transmis au CGI (HTTP_COOKIE)"
R=$(curl -s -b "gouter=chocolatine" "$BASE/cgi-bin/cookies.py")
if echo "$R" | grep -q '"gouter": "chocolatine"'; then
	ok "cookie 'gouter' vu par le CGI"
else
	ko "cookie absent côté CGI : $R"
fi

# ---------------------------------------------------------------- 2. Set-Cookie relayé ?
echo "[2] Le Set-Cookie émis par le CGI est relayé au client"
HDRS=$(curl -s -D - -o /dev/null "$BASE/cgi-bin/cookies.py?set=relay:ok")
if echo "$HDRS" | grep -qi '^Set-Cookie: *relay=ok'; then
	ok "header Set-Cookie présent dans la réponse"
else
	warn "Set-Cookie perdu : CGIReadHandler::parseCgi() ne relaie que Status/Content-Type"
fi

# ---------------------------------------------------------------- 3. création de session
echo "[3] Création de session (nouveau sid généré par le serveur)"
R=$(curl -s "$BASE/cgi-bin/session.py?action=visit")
SID=$(json_get "$R" "sid")
NEW=$(echo "$R" | grep -o '"new_session": *true')
if [ -n "$SID" ] && [ -n "$NEW" ]; then
	ok "nouvelle session sid=$SID"
else
	ko "pas de session créée : $R"
fi

# ---------------------------------------------------------------- 4. persistance du compteur
echo "[4] Le compteur de visites persiste côté serveur (même sid)"
V1=$(json_num "$(curl -s -b "sid=$SID" "$BASE/cgi-bin/session.py?action=visit")" "visits")
V2=$(json_num "$(curl -s -b "sid=$SID" "$BASE/cgi-bin/session.py?action=visit")" "visits")
V3=$(json_num "$(curl -s -b "sid=$SID" "$BASE/cgi-bin/session.py?action=visit")" "visits")
if [ -n "$V1" ] && [ "$V2" = "$((V1+1))" ] && [ "$V3" = "$((V1+2))" ]; then
	ok "visites : $V1 -> $V2 -> $V3"
else
	ko "compteur incohérent : $V1, $V2, $V3"
fi

# ---------------------------------------------------------------- 5. données de session
echo "[5] Stockage/relecture d'une donnée dans la session"
curl -s -b "sid=$SID" "$BASE/cgi-bin/session.py?action=set&key=nom&value=marvin" > /dev/null
R=$(curl -s -b "sid=$SID" "$BASE/cgi-bin/session.py?action=get")
if echo "$R" | grep -q '"nom": "marvin"'; then
	ok "nom=marvin relu depuis la session"
else
	ko "donnée perdue : $R"
fi

# ---------------------------------------------------------------- 6. session via POST
echo "[6] Les actions de session marchent aussi en POST (body urlencoded)"
R=$(curl -s -b "sid=$SID" -X POST -H "Content-Type: application/x-www-form-urlencoded" \
	--data "action=set&key=lang&value=fr" "$BASE/cgi-bin/session.py")
if echo "$R" | grep -q '"lang": "fr"'; then
	ok "action=set envoyée en POST"
else
	ko "POST session KO : $R"
fi

# ---------------------------------------------------------------- 7. isolation
echo "[7] Deux sids différents = deux sessions isolées"
OTHER=$(printf 'f%.0s' $(seq 1 32))
R=$(curl -s -b "sid=$OTHER" "$BASE/cgi-bin/session.py?action=get")
if echo "$R" | grep -q '"nom": "marvin"'; then
	ko "fuite de données entre sessions !"
else
	ok "l'autre session ne voit pas 'nom=marvin'"
fi

# ---------------------------------------------------------------- 8. logout
echo "[8] Logout : la session est détruite côté serveur"
curl -s -b "sid=$SID" "$BASE/cgi-bin/session.py?action=logout" > /dev/null
V=$(json_num "$(curl -s -b "sid=$SID" "$BASE/cgi-bin/session.py?action=visit")" "visits")
if [ "$V" = "1" ]; then
	ok "après logout, visits repart à 1"
else
	ko "session encore vivante après logout (visits=$V)"
fi

echo
echo "=== Résultat : $PASS PASS, $FAIL FAIL, $WARN WARN ==="
[ "$FAIL" -eq 0 ]
