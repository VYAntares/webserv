#!/usr/bin/env python3
# Bloc headers present (donc separateur trouve) mais sans Content-Type :
# la reponse finale ne doit pas avoir de header Content-Type du tout.
print("Status: 200 OK")
print()
print("NO_CTYPE_BODY")
