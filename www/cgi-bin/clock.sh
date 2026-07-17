#!/usr/bin/bash
# clock.sh — belle horloge HTML avec la query string pour le fuseau horaire
echo "Content-Type: text/html; charset=utf-8"
echo ""

# Parse QUERY_STRING pour extraire le décalage horaire
QS="$QUERY_STRING"
OFFSET="+0"
if echo "$QS" | grep -q "offset="; then
    OFFSET=$(echo "$QS" | sed 's/.*offset=//;s/&.*//')
fi

# Heure actuelle avec offset
if command -v python3 &>/dev/null; then
    NOW=$(python3 -c "
import datetime
d = datetime.datetime.utcnow() + datetime.timedelta(hours=$OFFSET)
print(d.strftime('%H:%M:%S'))
")
    DATER=$(python3 -c "
import datetime
d = datetime.datetime.utcnow() + datetime.timedelta(hours=$OFFSET)
jour = ['Dimanche','Lundi','Mardi','Mercredi','Jeudi','Vendredi','Samedi'][int(d.strftime('%w'))]
mois = ['janvier','février','mars','avril','mai','juin','juillet','août','septembre','octobre','novembre','décembre'][d.month-1]
print(f'{jour} {d.day} {mois} {d.year}')
")
else
    NOW=$(date "+%H:%M:%S")
    DATER=$(date "+%A %d %B %Y")
fi

cat <<EOF
<!DOCTYPE html>
<html lang="fr">
<head><meta charset="utf-8"><meta http-equiv="refresh" content="1"><title>Horloge</title>
<style>
  body { margin:0; min-height:100vh; display:flex; align-items:center; justify-content:center;
    background:radial-gradient(ellipse at center, #0d1020 0%, #060810 100%); font-family:'DM Sans',sans-serif; }
  .wrap { text-align:center; }
  .time { font-family:'Space Mono',monospace; font-size:7rem; color:#4ae0ff; text-shadow:0 0 40px rgba(74,224,255,.15); }
  .date { font-size:1.2rem; color:#8b89a0; margin-top:.5rem; font-family:'Instrument Serif',serif; }
  .tz { margin-top:2rem; display:flex; gap:.6rem; justify-content:center; flex-wrap:wrap; }
  .tz a { padding:.35rem .9rem; border-radius:99px; text-decoration:none; font-size:.75rem;
    color:#5c5a72; border:1px solid #2a2a42; }
  .tz a:hover, .tz a.active { border-color:#4ae0ff; color:#4ae0ff; }
</style></head>
<body><div class="wrap">
<div class="time">$NOW</div>
<div class="date">$DATER</div>
<div class="tz">
  <a href="?offset=0" class="active">UTC</a>
  <a href="?offset=1">Paris</a>
  <a href="?offset=-5">New York</a>
  <a href="?offset=9">Tokyo</a>
  <a href="?offset=8">Shanghai</a>
</div>
</div></body></html>
EOF
