#!/usr/bin/env python3
# Meme chose que test_status.py mais avec un vrai separateur \r\n\r\n.
import sys
sys.stdout.write("Status: 201 Created\r\n")
sys.stdout.write("Content-Type: text/plain\r\n")
sys.stdout.write("\r\n")
sys.stdout.write("STATUS_201_BODY\n")
