#!/usr/bin/env python3
import sys
import os

# Define standard line ending
CRLF = "\r\n"

# 1. SEND HEADERS
# We must send headers first, or the server will return 502
sys.stdout.write("Content-Type: text/plain" + CRLF)
sys.stdout.write(CRLF) # The Magic Empty Line

# 2. READ POST DATA
# We check how big the message is first
try:
    length = int(os.environ.get('CONTENT_LENGTH', 0))
except (ValueError, TypeError):
    length = 0

# We read exactly 'length' bytes from Standard Input
if length > 0:
    post_body = sys.stdin.read(length)
else:
    post_body = "Empty Body"

# 3. PRINT OUTPUT
sys.stdout.write("--- CGI POST TEST ---" + CRLF)
sys.stdout.write("Hello World" + CRLF)
sys.stdout.write(f"Received Body: {post_body}" + CRLF)