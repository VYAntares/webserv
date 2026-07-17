#!/usr/bin/bash
# guestbook.sh — CGI POST : recoit un message et l'affiche joliment
echo "Content-Type: text/html; charset=utf-8"
echo ""

# Lire le body depuis stdin
BODY=""
if [ -n "$CONTENT_LENGTH" ] && [ "$CONTENT_LENGTH" -gt 0 ]; then
    read -r -n "$CONTENT_LENGTH" BODY
fi

# Parser urlencoded
nom="Anonyme"
ville="Inconnue"
message=""

# Extraire les champs
for pair in $(echo "$BODY" | tr '&' ' '); do
    key=$(echo "$pair" | cut -d= -f1)
    val=$(echo "$pair" | cut -d= -f2- | python3 -c "import sys,urllib.parse; print(urllib.parse.unquote_plus(sys.stdin.read().strip()))" 2>/dev/null || echo "$pair" | cut -d= -f2- | sed 's/+/ /g;s/%/\\x/g' | xargs -0 printf "%b" 2>/dev/null || echo "$pair" | cut -d= -f2-)
    case "$key" in
        nom) nom="$val" ;;
        ville) ville="$val" ;;
        message) message="$val" ;;
    esac
done

# Couleur aléatoire selon la ville (hash simple)
HASH=$(echo "$ville" | md5sum 2>/dev/null | cut -c1-6 || echo "4ae0ff")
COLOR="#${HASH:-4ae0ff}"

cat <<EOF
<!DOCTYPE html>
<html lang="fr">
<head><meta charset="utf-8"><title>Livre d'or</title>
<style>
  body { margin:0; min-height:100vh; display:flex; align-items:center; justify-content:center;
    background:#0a0a0f; font-family:'DM Sans',sans-serif; }
  .card { background:#12121a; border:1px solid #2a2a42; border-radius:16px; padding:2.5rem; max-width:520px; }
  .pin { width:12px; height:12px; border-radius:50%; background:$COLOR; margin-bottom:1.5rem;
    box-shadow:0 0 12px ${COLOR}44; }
  .city { font-family:'Instrument Serif',serif; font-size:2.2rem; color:#e8e6f0; margin:0 0 .3rem; }
  .from { color:#8b89a0; font-size:.9rem; margin-bottom:1.5rem; }
  .message { background:#1a1a28; border-left:3px solid $COLOR; padding:1rem 1.2rem; border-radius:0 10px 10px 0;
    color:#e8e6f0; font-size:1rem; line-height:1.7; font-style:italic; margin-bottom:1.5rem; }
  .meta { font-family:'Space Mono',monospace; font-size:.65rem; color:#5c5a72; }
  a { color:#4ae0ff; text-decoration:none; font-size:.8rem; }
</style></head>
<body><div class="card">
<div class="pin"></div>
EOF

echo "<h2 class=\"city\">$ville</h2>"
echo "<p class=\"from\">Un message de <strong>$nom</strong></p>"

if [ -n "$message" ]; then
    echo "<div class=\"message\">&ldquo;$message&rdquo;</div>"
else
    echo "<div class=\"message\">&ldquo;... juste un coucou de $ville !&rdquo;</div>"
fi

cat <<EOF
<p class="meta">POST CGI — Bash — $CONTENT_LENGTH octets reçus</p>
<a href="/default/cgi.html">← Retour aux tests CGI</a>
</div></body></html>
EOF
