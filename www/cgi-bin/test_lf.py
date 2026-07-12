#!/usr/bin/env python3
# Script qui n'emet que des \n (print() standard) : doit retomber sur le
# fallback \n\n de buildCgiResponse().
print("Content-Type: text/plain")
print()
print("LF_BODY_OK")
