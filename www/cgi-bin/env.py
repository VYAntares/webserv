#!/usr/bin/env python3
# Dump de l'environnement CGI + body reçu, en JSON.
# Sert à vérifier que le serveur transmet bien toutes les variables CGI
# (QUERY_STRING, REQUEST_METHOD, CONTENT_LENGTH, HTTP_*, ...).
import os
import sys
import json

length = int(os.environ.get("CONTENT_LENGTH", "0") or "0")
body = sys.stdin.buffer.read() if length or os.environ.get("REQUEST_METHOD") == "POST" else b""

out = {
    "env": {k: v for k, v in sorted(os.environ.items())},
    "body_bytes_read": len(body),
    "body_preview": body[:200].decode("utf-8", "replace"),
}

payload = json.dumps(out, indent=2)
sys.stdout.write("Content-Type: application/json\r\n\r\n")
sys.stdout.write(payload)
