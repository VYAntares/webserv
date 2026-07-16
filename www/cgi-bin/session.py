#!/usr/bin/env python3
# ============================================================================
# BONUS : gestion de sessions côté serveur, basée sur un cookie "sid".
#
# - Le client présente un cookie sid (header Cookie -> variable HTTP_COOKIE).
# - L'état de session est stocké côté serveur dans ./sessions/<sid>.json
#   (le serveur fait chdir() dans le dossier du script => chemin relatif ok).
# - Si aucun sid n'est présent, un nouveau est généré et renvoyé :
#     * via un header "Set-Cookie" (flux cookie standard),
#     * ET dans le body JSON (fallback si le serveur ne relaie pas Set-Cookie).
#
# Actions (query string ou body urlencoded) :
#   action=visit           incrémente le compteur de visites (défaut)
#   action=set&key=K&value=V  stocke K=V dans la session
#   action=get             lit la session sans la modifier
#   action=logout          détruit la session (+ cookie expiré)
# ============================================================================
import os
import sys
import json
import re
import time
import uuid
from urllib.parse import parse_qs

SESS_DIR = "sessions"
SESS_TTL = 3600  # secondes


def parse_cookies(raw):
    cookies = {}
    for part in raw.split(";"):
        if "=" in part:
            k, _, v = part.strip().partition("=")
            cookies[k] = v
    return cookies


def sess_path(sid):
    return os.path.join(SESS_DIR, sid + ".json")


def load(sid):
    try:
        with open(sess_path(sid)) as f:
            return json.load(f)
    except Exception:
        return None


def save(sid, data):
    if not os.path.isdir(SESS_DIR):
        os.makedirs(SESS_DIR)
    with open(sess_path(sid), "w") as f:
        json.dump(data, f)


def cleanup():
    now = time.time()
    if not os.path.isdir(SESS_DIR):
        return
    for name in os.listdir(SESS_DIR):
        p = os.path.join(SESS_DIR, name)
        try:
            if now - os.path.getmtime(p) > SESS_TTL:
                os.unlink(p)
        except OSError:
            pass


# ---- paramètres : query string + body urlencoded --------------------------
params = {}
for k, v in parse_qs(os.environ.get("QUERY_STRING", "")).items():
    params[k] = v[0]
length = int(os.environ.get("CONTENT_LENGTH", "0") or "0")
body = sys.stdin.buffer.read()
if "application/x-www-form-urlencoded" in os.environ.get("CONTENT_TYPE", ""):
    for k, v in parse_qs(body.decode("utf-8", "replace")).items():
        params[k] = v[0]

action = params.get("action", "visit")
cookies = parse_cookies(os.environ.get("HTTP_COOKIE", ""))
sid = cookies.get("sid", "")

cleanup()

new_session = False
if not re.match(r"^[0-9a-f]{32}$", sid):
    sid = ""
if not sid:
    sid = uuid.uuid4().hex
    new_session = True

session = load(sid) or {"created_at": time.strftime("%Y-%m-%d %H:%M:%S"), "visits": 0}

set_cookie = None
if new_session:
    set_cookie = "sid=%s; Path=/" % sid

if action == "visit":
    session["visits"] = session.get("visits", 0) + 1
    save(sid, session)
elif action == "set":
    key = params.get("key", "")
    if key:
        session[key] = params.get("value", "")
    save(sid, session)
elif action == "get":
    pass
elif action == "logout":
    try:
        os.unlink(sess_path(sid))
    except OSError:
        pass
    session = {}
    set_cookie = "sid=deleted; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT"

out = {
    "sid": sid,
    "new_session": new_session,
    "action": action,
    "session": session,
    "cookies_received": cookies,
    "set_cookie_sent": set_cookie,
}

sys.stdout.write("Content-Type: application/json\r\n")
if set_cookie:
    sys.stdout.write("Set-Cookie: %s\r\n" % set_cookie)
sys.stdout.write("\r\n")
sys.stdout.write(json.dumps(out, indent=2))
