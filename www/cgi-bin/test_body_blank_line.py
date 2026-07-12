#!/usr/bin/env python3
# Verifie que seul le PREMIER \n\n (separateur headers/body) est consomme :
# la ligne vide presente plus loin dans le body doit rester intacte.
print("Content-Type: text/plain")
print()
print("first paragraph")
print()
print("second paragraph")
