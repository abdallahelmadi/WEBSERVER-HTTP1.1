# import os
# import sys

# # Define line ending
# CRLF = "\r\n"

# # 1. Send Headers
# sys.stdout.write("Content-Type: text/html" + CRLF)
# # Optional: Status header to help your parser
# sys.stdout.write("Status: 200 OK" + CRLF) 

# # 2. End of Headers (The Empty Line)
# sys.stdout.write(CRLF)

# # 3. The Body - Printing the Environment Variables
# sys.stdout.write("<html><body>")
# sys.stdout.write("<h1>CGI GET Test Debugger</h1>")

# # Check REQUEST_METHOD
# method = os.environ.get("REQUEST_METHOD", "NOT_FOUND")
# sys.stdout.write(f"<p><b>Request Method:</b> {method}</p>" + CRLF)

# # Check QUERY_STRING
# query = os.environ.get("QUERY_STRING", "NOT_FOUND")
# sys.stdout.write(f"<p><b>Query String:</b> {query}</p>" + CRLF)

# sys.stdout.write("</body></html>")

# test 2 for the cgi environment variables

#!/usr/bin/env python3
import os
import sys

# Define standard line ending
CRLF = "\r\n"

# 1. Always output headers first so the server doesn't crash on 502
sys.stdout.write("Content-Type: text/html" + CRLF)
sys.stdout.write("Status: 200 OK" + CRLF)
sys.stdout.write(CRLF)

# 2. READ THE ENVIRONMENT
method = os.environ.get("REQUEST_METHOD", "")
query_string = os.environ.get("QUERY_STRING", "")

# 3. LOGIC: The "Gatekeeper" Check
sys.stdout.write("<html><body>")

# Check 1: Did the server tell us the Method?
if method != "GET":
    sys.stdout.write(f"<h1>FAILURE: Wrong Method</h1>")
    sys.stdout.write(f"<p>Expected 'GET', but server sent: '{method}'</p>")
    sys.stdout.write("</body></html>")
    sys.exit(0)

# Check 2: Did the server pass the Query String?
# We require the password "secret=123" to be in the URL
if "secret=123" not in query_string:
    sys.stdout.write(f"<h1>FAILURE: Missing or Wrong Data</h1>")
    sys.stdout.write(f"<p>I expected '?secret=123' in the URL.</p>")
    sys.stdout.write(f"<p>Server sent QUERY_STRING: '{query_string}'</p>")
    sys.stdout.write("</body></html>")
    sys.exit(0)

# 4. SUCCESS
sys.stdout.write("<h1>SUCCESS!</h1>")
sys.stdout.write("<p>Your C++ Server is correctly passing environment variables.</p>")
sys.stdout.write("</body></html>")