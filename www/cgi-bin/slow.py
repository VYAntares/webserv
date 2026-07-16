#!/usr/bin/env python3
# CGI trop lent : dort plus longtemps que le timeout serveur (30 s)
# -> le serveur doit répondre 504 Gateway Timeout, sans jamais bloquer
#    les autres clients pendant l'attente.
# ?sleep=N pour changer la durée (défaut 35 s).
import os
import sys
import time
from urllib.parse import parse_qs

sys.stdin.buffer.read()
params = {k: v[0] for k, v in parse_qs(os.environ.get("QUERY_STRING", "")).items()}
duration = int(params.get("sleep", "35"))
time.sleep(duration)

sys.stdout.write("Content-Type: text/plain\r\n\r\n")
sys.stdout.write("Je me suis reveille apres %d s (le timeout serveur n'a pas fonctionne !)\n" % duration)
