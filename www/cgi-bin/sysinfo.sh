#!/usr/bin/bash
# sysinfo.sh — affiche une page HTML avec infos systeme
echo "Content-Type: text/html; charset=utf-8"
echo ""

HOST=$(hostname 2>/dev/null || echo "webserv")
UPTIME=$(uptime -p 2>/dev/null | sed 's/^up //' || echo "inconnu")
DATE=$(date "+%d %B %Y, %H:%M:%S" 2>/dev/null || date)

cat <<EOF
<!DOCTYPE html>
<html lang="fr">
<head><meta charset="utf-8"><title>Infos système</title>
<style>
  body { margin:0; min-height:100vh; display:flex; align-items:center; justify-content:center;
    background:#0a0a0f; font-family:'DM Sans',sans-serif; }
  .card { background:#12121a; border:1px solid #2a2a42; border-radius:16px; padding:2.5rem; max-width:480px; }
  h2 { color:#c084fc; font-family:'Instrument Serif',serif; font-size:2rem; margin:0 0 1.5rem; }
  .row { display:flex; justify-content:space-between; padding:.6rem 0; border-bottom:1px solid #1a1a28; }
  .row .label { color:#8b89a0; font-size:.85rem; }
  .row .value { color:#e8e6f0; font-family:'Space Mono',monospace; font-size:.82rem; }
</style></head>
<body>
<div class="card">
  <h2>Infos système</h2>
  <div class="row"><span class="label">Machine</span><span class="value">$HOST</span></div>
  <div class="row"><span class="label">Uptime</span><span class="value">$UPTIME</span></div>
  <div class="row"><span class="label">Date serveur</span><span class="value">$DATE</span></div>
  <div class="row"><span class="label">Shell</span><span class="value">$SHELL</span></div>
  <div class="row"><span class="label">User</span><span class="value">$USER</span></div>
  <div class="row"><span class="label">Requête</span><span class="value">$REQUEST_METHOD $SCRIPT_NAME</span></div>
</div>
</body></html>
EOF
