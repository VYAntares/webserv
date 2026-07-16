#!/usr/bin/env python3
# CGI tué par un signal (SIGSEGV) -> le serveur doit répondre 502 Bad Gateway.
import os
import sys
import signal
sys.stdin.buffer.read()
os.kill(os.getpid(), signal.SIGSEGV)
