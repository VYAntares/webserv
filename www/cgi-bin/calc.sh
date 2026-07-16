#!/usr/bin/env bash
# BONUS "multiple CGI types" : CGI shell (bash), en plus des CGI Python.
# GET  /cgi-bin/calc.sh?a=6&b=7  -> additionne a et b
# POST                            -> échote aussi le body lu sur stdin (EOF)

# lire le body (EOF = fin, le serveur a dé-chunké si besoin)
BODY=""
if [ "$REQUEST_METHOD" = "POST" ]; then
	BODY=$(cat)
fi

# extraire a et b de QUERY_STRING (format a=X&b=Y, pas de regex compliquée)
A=0; B=0
for kv in ${QUERY_STRING//&/ }; do
	case "$kv" in
		a=*) A="${kv#a=}";;
		b=*) B="${kv#b=}";;
	esac
done
SUM=$(( A + B ))

printf 'Content-Type: application/json\r\n\r\n'
printf '{\n'
printf '  "language": "bash",\n'
printf '  "script": "calc.sh",\n'
printf '  "method": "%s",\n' "$REQUEST_METHOD"
printf '  "query_string": "%s",\n' "$QUERY_STRING"
printf '  "a": %s, "b": %s, "sum": %s,\n' "$A" "$B" "$SUM"
printf '  "body_bytes_read": %s,\n' "${#BODY}"
printf '  "server_software": "%s"\n' "$SERVER_SOFTWARE"
printf '}\n'
