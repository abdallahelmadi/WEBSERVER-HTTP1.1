import sys

CRLF = "\r\n"
# Write HTTP headers

sys.stdout.write("Content-Type: text/plain" + CRLF)
sys.stdout.write("Content-Length:  12" + CRLF)
sys.stdout.write("ikhan: hhhh" + CRLF)

sys.stdout.write(CRLF)  # End of headers
# Write HTTP body
sys.stdout.write("Hello! I am the body, and now I am correctly parsed.")
