#!/usr/bin/env python3
# CGI qui échoue (exit code != 0) -> le serveur doit répondre 500.
import sys
sys.stdin.buffer.read()
sys.exit(1)
