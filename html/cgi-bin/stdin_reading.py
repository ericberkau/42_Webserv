#!/usr/bin/env python3
import sys
import os

content_length = int(os.environ.get("CONTENT_LENGTH", 0))
body = sys.stdin.read(content_length) if content_length > 0 else ""

print("Content-Type: text/plain")
print()
print("REQUEST_METHOD =", os.environ.get("REQUEST_METHOD"))
print("CONTENT_LENGTH =", os.environ.get("CONTENT_LENGTH"))
print("BODY           =", body)