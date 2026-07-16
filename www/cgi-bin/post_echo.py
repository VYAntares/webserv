#!/usr/bin/env python3
# Echo d'un POST : renvoie en JSON le nombre d'octets lus sur stdin, le md5,
# et les champs si le body est application/x-www-form-urlencoded.
# Sert à valider : POST vers CGI, gros bodies, et le dé-chunking fait par le
# serveur (le CGI lit jusqu'à EOF, il ne doit JAMAIS voir de framing chunked).
import os
import sys
import json
import hashlib
from urllib.parse import parse_qs

body = sys.stdin.buffer.read()  # EOF = fin du body (exigence du sujet)

fields = {}
ctype = os.environ.get("CONTENT_TYPE", "")
if "application/x-www-form-urlencoded" in ctype:
    try:
        parsed = parse_qs(body.decode("utf-8", "replace"))
        fields = {k: v[0] if len(v) == 1 else v for k, v in parsed.items()}
    except Exception:
        pass

looks_chunked = body[:16].split(b"\r\n")[0].strip().isdigit() is False and b"\r\n" in body[:8]

out = {
    "method": os.environ.get("REQUEST_METHOD", ""),
    "content_type": ctype,
    "content_length_env": os.environ.get("CONTENT_LENGTH", ""),
    "body_bytes_read": len(body),
    "body_md5": hashlib.md5(body).hexdigest(),
    "body_preview": body[:120].decode("utf-8", "replace"),
    "fields": fields,
    "query_string": os.environ.get("QUERY_STRING", ""),
}

payload = json.dumps(out, indent=2)
sys.stdout.write("Content-Type: application/json\r\n\r\n")
sys.stdout.write(payload)
