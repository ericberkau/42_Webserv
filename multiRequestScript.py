import socket

HOST = "127.0.0.1"
PORT = 8080

payload = (
    "GET /images/ HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "\r\n"
    "GET / HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "\r\n"

)

with socket.create_connection((HOST, PORT)) as sock:
    sock.settimeout(1.0)

    sock.sendall(payload.encode())

    response = b""

    while True:
        try:
            chunk = sock.recv(4096)
            if not chunk:
                break
            response += chunk
        except socket.timeout:
            # print("!!!! loop broke manually")
            break

print("Received", len(response), "bytes")
print(response.decode(errors="replace"))