#!/usr/bin/python3
import os, random

name = "Palette"
qs = os.environ.get("QUERY_STRING", "")
count = int(qs.split("=")[-1]) if "=" in qs else 5
count = max(3, min(count, 12))

colors = []
for _ in range(count):
    r, g, b = random.randint(60, 240), random.randint(60, 240), random.randint(60, 240)
    hexc = f"#{r:02x}{g:02x}{b:02x}"
    colors.append((hexc, r, g, b))

print("Content-Type: text/html; charset=utf-8\n")
print("""<!DOCTYPE html>
<html lang="fr">
<head><meta charset="utf-8"><title>Palette</title>
<style>
  body { margin:0; min-height:100vh; display:flex; align-items:center; justify-content:center;
    background:#0a0a0f; font-family:'DM Sans',sans-serif; }
  .wrap { max-width:700px; padding:2rem; text-align:center; }
  h2 { color:#e8e6f0; font-family:'Instrument Serif',serif; font-size:2rem; margin-bottom:1.5rem; }
  .grid { display:flex; gap:.4rem; justify-content:center; flex-wrap:wrap; margin-bottom:1.5rem; }
  .swatch { width:80px; height:80px; border-radius:12px; position:relative; }
  .swatch .hex { position:absolute; bottom:6px; left:0; right:0; text-align:center;
    font-size:.6rem; font-family:'Space Mono',monospace; color:#fff; text-shadow:0 1px 2px rgba(0,0,0,.6); }
  .swatch .rgb { position:absolute; top:6px; left:0; right:0; text-align:center;
    font-size:.55rem; font-family:'Space Mono',monospace; color:rgba(255,255,255,.7); }
  .btn { display:inline-block; padding:.4rem 1rem; border-radius:6px; text-decoration:none;
    font-size:.8rem; color:#8b89a0; border:1px solid #2a2a42; margin:.2rem; }
  .btn:hover { border-color:#4ae0ff; color:#4ae0ff; }
</style></head>
<body><div class="wrap">
<h2>Palette aléatoire</h2>
<div class="grid">""")

for hexc, r, g, b in colors:
    print(f'  <div class="swatch" style="background:{hexc}"><span class="rgb">R{r} G{g} B{b}</span><span class="hex">{hexc}</span></div>')

print("""</div>
<div>""")
for n in [3,5,8,12]:
    print(f'  <a href="?n={n}" class="btn">{n} couleurs</a>')
print("""  <a href="?" class="btn">↻ Autre</a>
</div>
</div></body></html>""")
