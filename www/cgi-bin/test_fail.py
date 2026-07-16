#!/usr/bin/env python3
# Exit non-nul : CGIReadHandler doit ignorer buildCgiResponse() et renvoyer
# une 500 (ErrorHandler), sans jamais exposer STATUS_SHOULD_NOT_APPEAR au
# client comme un 200.
import sys
print("Content-Type: text/plain")
print()
print("STATUS_SHOULD_NOT_APPEAR")
sys.exit(1)
