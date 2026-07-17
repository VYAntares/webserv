#!/usr/bin/python3
import os, random, html

quotes = [
    ("« Simplicity is prerequisite for reliability. »", "Edsger Dijkstra"),
    ("« Debugging is twice as hard as writing the code. »", "Brian Kernighan"),
    ("« First solve the problem, then write the code. »", "John Johnson"),
    ("« Any fool can write code a computer can understand. »", "Martin Fowler"),
    ("« Talk is cheap. Show me the code. »", "Linus Torvalds"),
    ("« The best error message is the one that never shows up. »", "Thomas Fuchs"),
    ("« Weeks of coding can save hours of planning. »", "Unknown"),
    ("« There are two ways of constructing software. »", "Tony Hoare"),
]

idx = hash(os.environ.get("QUERY_STRING", "")) % len(quotes)
if idx < 0: idx = -idx
quote, author = quotes[idx]

qs = os.environ.get("QUERY_STRING", "")
color = qs if qs in ("cyan","purple","green","orange") else "cyan"
theme = {"cyan": ("#0a2a2a","#4ae0ff"), "purple": ("#1a0a2a","#c084fc"),
         "green": ("#0a2a0a","#4ade80"), "orange": ("#2a0a08","#ff6b4a")}.get(color, ("#0a0a0f","#4ae0ff"))

print("Content-Type: text/html; charset=utf-8\n")
print(f"""<!DOCTYPE html>
<html lang="fr">
<head><meta charset="utf-8"><title>Citation</title>
<style>
  body {{ margin:0; min-height:100vh; display:flex; align-items:center; justify-content:center;
    background:{theme[0]}; font-family:Georgia,serif; }}
  .card {{ max-width:600px; padding:3rem 2.5rem; text-align:center; }}
  .quote {{ font-size:1.6rem; color:{theme[1]}; line-height:1.5; font-style:italic; margin-bottom:1.2rem; }}
  .author {{ color:#aaa; font-size:0.95rem; }}
  .tags {{ margin-top:2rem; display:flex; gap:.5rem; justify-content:center; flex-wrap:wrap; }}
  .tag {{ padding:.3rem .8rem; border-radius:99px; text-decoration:none; font-size:.75rem;
    background:rgba(255,255,255,0.05); border:1px solid rgba(255,255,255,0.1); color:#999; }}
  .tag:hover {{ border-color:{theme[1]}; color:{theme[1]}; }}
</style></head>
<body>
<div class="card">
  <p class="quote">{html.escape(quote)}</p>
  <p class="author">&mdash; {html.escape(author)}</p>
  <div class="tags">
    <a href="?cyan" class="tag">cyan</a>
    <a href="?purple" class="tag">violet</a>
    <a href="?green" class="tag">vert</a>
    <a href="?orange" class="tag">orange</a>
  </div>
</div>
</body></html>""")
