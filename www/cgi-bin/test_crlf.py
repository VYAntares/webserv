#!/usr/bin/env python3
# Cas nominal RFC 3875 : separateur headers/body en \r\n\r\n.
import sys
sys.stdout.write("Content-Type: text/plain\r\n")
sys.stdout.write("\r\n")
sys.stdout.write("CRLF_BODY_OK\n")
