#!/usr/bin/python3
"""feedback.py — CGI POST : recoit un formulaire et renvoie une belle page de confirmation."""
import os, sys, html

content_length = int(os.environ.get("CONTENT_LENGTH", "0"))
body = sys.stdin.read(content_length) if content_length > 0 else ""

# Parse urlencoded body
fields = {}
for pair in body.split("&"):
    if "=" in pair:
        k, v = pair.split("=", 1)
        from urllib.parse import unquote_plus
        fields[k] = unquote_plus(v)

nom = fields.get("nom", "Anonyme")
msg = fields.get("message", "").strip()
note = fields.get("note", "5")
emoji = {"1": "😡","2": "😕","3": "😐","4": "😊","5": "🤩"}.get(note, "😐")

stars = int(note) if note.isdigit() else 3

color_map = {"5": "#4ade80", "4": "#a3e635", "3": "#facc15", "2": "#fb923c", "1": "#ff6b4a"}
color = color_map.get(note, "#4ade80")

print("Content-Type: text/html; charset=utf-8\n")
print(f"""<!DOCTYPE html>
<html lang="fr">
<head><meta charset="utf-8"><title>Merci !</title>
<style>
  body {{ margin:0; min-height:100vh; display:flex; align-items:center; justify-content:center;
    background:#0a0a0f; font-family:'DM Sans',sans-serif; }}
  .card {{ background:#12121a; border:1px solid #2a2a42; border-radius:16px; padding:2.5rem 3rem;
    max-width:500px; text-align:center; }}
  .emoji {{ font-size:3rem; margin-bottom:.8rem; }}
  h2 {{ color:{color}; font-family:'Instrument Serif',serif; font-size:2rem; margin:0 0 .5rem; }}
  .msg {{ color:#8b89a0; font-size:.95rem; line-height:1.6; }}
  .field {{ margin-top:1.5rem; padding:1rem; background:#1a1a28; border-radius:10px; text-align:left; }}
  .field .label {{ color:#5c5a72; font-size:.7rem; text-transform:uppercase; letter-spacing:.1em; }}
  .field .value {{ color:#e8e6f0; font-size:.9rem; margin-top:.2rem; font-style:italic; }}
  .stars {{ margin:.8rem 0; font-size:1.2rem; letter-spacing:.2em; }}
  .stars .on {{ color:{color}; }} .stars .off {{ color:#2a2a42; }}
  a {{ display:inline-block; margin-top:1.5rem; color:#4ae0ff; text-decoration:none; font-size:.82rem; }}
  a:hover {{ text-decoration:underline; }}
</style></head>
<body><div class="card">
<div class="emoji">{emoji}</div>
<h2>Merci {html.escape(nom)} !</h2>
<div class="stars">{''.join(f'<span class="on">★</span>' for _ in range(stars))}{''.join(f'<span class="off">★</span>' for _ in range(5-stars))}</div>""")

if msg:
    print(f'<div class="field"><div class="label">Votre message</div><div class="value">&ldquo;{html.escape(msg)}&rdquo;</div></div>')

print(f"""<p class="msg" style="margin-top:1rem;">Note enregistrée : <strong>{note}/5</strong></p>
<a href="/default/cgi.html">← Retour aux tests CGI</a>
</div></body></html>""")
