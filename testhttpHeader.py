import socket
import time

HOST = "127.0.0.1"
PORT = 8080
TIMEOUT = 1.5

# Define test cases: (Test Name, HTTP Payload, Expected Status Code)
TEST_CASES = [
    (
        "VALID: Standard Host Header",
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n",
        "200" # Or 301/404 depending on your root directory files
    ),
    (
        "INVALID: Broken Host key (missing characters)",
        "GET / HTTP/1.1\r\n"
        "Hos: localhost\r\n"  # 'Host' is missing 't'
        "\r\n",
        "400"
    ),
    (
        "INVALID: Missing Host Header entirely",
        "GET / HTTP/1.1\r\n"
        "User-Agent: TestClient\r\n"
        "\r\n",
        "400"
    ),
    (
        "INVALID: Space before the colon (Strict HTTP rule)",
        "GET / HTTP/1.1\r\n"
        "Host : localhost\r\n"  # Space before ':' is an immediate 400
        "\r\n",
        "400"
    ),
    (
        "INVALID: Duplicate Host Headers",
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Host: anotherhost.com\r\n"
        "\r\n",
        "400"
    ),
    (
        "VALID: Duplicate content-length Headers with same value",
        "GET / HTTP/1.1\r\n"
        "Content-Length: 1\r\n"
        "Content-Length: 1\r\n"
        "\r\n",
        "1",
        "200"
    ),
    (
        "INVALID: Duplicate Content-length Headers with different values",
        "GET / HTTP/1.1\r\n"
        "Content-Length: 1\r\n"
        "Content-Length: 5\r\n"
        "\r\n",
        "400"
    )
]

def run_test(name, payload, expected):
    print(f"=== Running Test: {name} ===")
    print("--- Sending Request ---")
    print(payload.replace("\r\n", "\\r\\n\n"), end="")
    print("-----------------------")
    
    try:
        # Create socket connection
        with socket.create_connection((HOST, PORT)) as sock:
            sock.settimeout(TIMEOUT)
            sock.sendall(payload.encode('utf-8'))
            
            response = b""
            while True:
                try:
                    chunk = sock.recv(4096)
                    if not chunk:
                        break
                    response += chunk
                except socket.timeout:
                    break
            
            # Parse the first line of the response to see the status code
            res_text = response.decode('utf-8', errors='replace')
            first_line = res_text.split("\r\n")[0]
            
            print(f"--- Server Response (First Line) ---")
            print(f"{first_line if first_line else '[No Response / Socket Closed]'}")
            
            # Simple validation check
            if expected in first_line:
                print("\033[92mRESULT: PASS\033[0m\n")
            else:
                print(f"\033[91mRESULT: FAIL (Expected status containing '{expected}')\033[0m\n")
                
    except ConnectionRefusedError:
        print("\033[91mError: Server is not running on port 8080!\033[0m\n")

if __name__ == "__main__":
    for name, payload, expected in TEST_CASES:
        run_test(name, payload, expected)
        time.sleep(0.1)  # Brief pause between connections





# import socket

# HOST = "127.0.0.1"
# PORT = 8080  # match your config's listen port

# def send_raw(payload: bytes, label: str):
#     print(f"=== {label} ===")
#     with socket.create_connection((HOST, PORT)) as sock:
#         sock.settimeout(1.0)
#         sock.sendall(payload)
#         response = b""
#         try:
#             while True:
#                 chunk = sock.recv(4096)
#                 if not chunk:
#                     break
#                 response += chunk
#         except socket.timeout:
#             pass
#     if response:
#         first_line = response.split(b"\r\n", 1)[0]
#         print("Status line:", first_line.decode(errors="replace"))
#     else:
#         print("Status line: <no response — connection hung or closed with nothing sent>")
#     print()

# # Case 1: exact duplicate, same value, WITH the body byte actually sent
# send_raw(
#     b"GET / HTTP/1.1\r\n"
#     b"Host: localhost\r\n"
#     b"Content-Length: 1\r\n"
#     b"Content-Length: 1\r\n"
#     b"\r\n"
#     b"a",  # the one body byte the header promises
#     "Same value, body sent -> expect 200"
# )

# # Case 2: conflicting values -> must reject
# send_raw(
#     b"GET / HTTP/1.1\r\n"
#     b"Host: localhost\r\n"
#     b"Content-Length: 1\r\n"
#     b"Content-Length: 5\r\n"
#     b"\r\n"
#     b"a",
#     "Conflicting values -> expect 400"
# )

# # Case 3: same value, headers only, NO body byte sent -> should hang, not error
# send_raw(
#     b"GET / HTTP/1.1\r\n"
#     b"Host: localhost\r\n"
#     b"Content-Length: 1\r\n"
#     b"Content-Length: 1\r\n"
#     b"\r\n",
#     "Same value, NO body sent -> expect no response before timeout (parser waiting)"
# )

# # Case 4: duplicate Host -> must reject
# send_raw(
#     b"GET / HTTP/1.1\r\n"
#     b"Host: localhost\r\n"
#     b"Host: localhost\r\n"
#     b"\r\n",
#     "Duplicate Host -> expect 400"
# )

# # Case 5: sanity check — totally normal request, no duplicates at all
# send_raw(
#     b"GET / HTTP/1.1\r\n"
#     b"Host: localhost\r\n"
#     b"\r\n",
#     "Normal request, no duplicates -> expect 200 (or your real default response)"
# )