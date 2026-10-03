#!/usr/bin/env python3

import os
import sys

print("Content-Type: text/plain")
print()

print("Hello from CGI!")
print()
print("REQUEST_METHOD =", os.environ.get("REQUEST_METHOD"))
print("QUERY_STRING   =", os.environ.get("QUERY_STRING"))
print("SCRIPT_NAME    =", os.environ.get("SCRIPT_NAME"))
print("PATH_INFO      =", os.environ.get("PATH_INFO"))
length = int(os.environ.get("CONTENT_LENGTH") or 0)
print("BODY           =", sys.stdin.read(length))