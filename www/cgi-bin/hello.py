#!/usr/bin/env python3
import os
import sys

body = sys.stdin.read()

print("Content-Type: text/html")
print()
print("<html><body>")
print("<h1>Hello from CGI!</h1>")
print("<p>REQUEST_METHOD = %s</p>" % os.environ.get("REQUEST_METHOD", ""))
print("<p>QUERY_STRING = %s</p>" % os.environ.get("QUERY_STRING", ""))
print("<p>body (%d bytes) = %s</p>" % (len(body), body))
print("</body></html>")
