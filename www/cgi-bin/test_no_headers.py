#!/usr/bin/env python3
# Aucun bloc headers du tout (pas de ligne vide) : buildCgiResponse() ne
# trouve ni \r\n\r\n ni \n\n, donc toute la sortie doit devenir le body tel
# quel, avec un code 200 par defaut.
import sys
sys.stdout.write("RAW_BODY_NO_HEADERS\n")
