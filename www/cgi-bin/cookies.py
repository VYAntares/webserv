#!/usr/bin/env python3
# BONUS cookies : montre les cookies reçus (HTTP_COOKIE) et peut en poser.
#   ?set=nom:valeur   -> émet "Set-Cookie: nom=valeur; Path=/"
#   ?del=nom          -> émet un Set-Cookie expiré pour supprimer le cookie
# Sans paramètre : liste simplement les cookies reçus.
import os
import sys
import json
from urllib.parse import parse_qs

sys.stdin.buffer.read()

params = {k: v[0] for k, v in parse_qs(os.environ.get("QUERY_STRING", "")).items()}

cookies = {}
for part in os.environ.get("HTTP_COOKIE", "").split(";"):
    if "=" in part:
        k, _, v = part.strip().partition("=")
        cookies[k] = v

set_cookie = None
if "set" in params and ":" in params["set"]:
    name, _, value = params["set"].partition(":")
    set_cookie = "%s=%s; Path=/" % (name, value)
elif "del" in params:
    set_cookie = "%s=deleted; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT" % params["del"]

out = {
    "cookies_received": cookies,
    "raw_http_cookie": os.environ.get("HTTP_COOKIE", ""),
    "set_cookie_sent": set_cookie,
}

sys.stdout.write("Content-Type: application/json\r\n")
if set_cookie:
    sys.stdout.write("Set-Cookie: %s\r\n" % set_cookie)
sys.stdout.write("\r\n")
sys.stdout.write(json.dumps(out, indent=2))
