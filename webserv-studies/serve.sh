#!/bin/bash
PORT=8080
DIR="$(cd "$(dirname "$0")" && pwd)"
IP=$(ipconfig getifaddr en0 2>/dev/null || ipconfig getifaddr en1 2>/dev/null || echo "IP introuvable")

echo ""
echo "  Mac    →  http://localhost:$PORT/webserv-mindmap.html"
echo "  iPhone →  http://$IP:$PORT/webserv-mindmap.html"
echo ""
echo "  (iPhone doit être sur le même Wi-Fi)"
echo "  Ctrl+C pour arrêter."
echo ""

open "http://localhost:$PORT/webserv-mindmap.html" 2>/dev/null &
python3 -m http.server $PORT --directory "$DIR"
