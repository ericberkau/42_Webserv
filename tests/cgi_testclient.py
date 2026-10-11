import socket

HOST = "127.0.0.1"
PORT = 8080

requests = {
    "GET CGI": (
        "GET /cgi-bin/cgi_testscript.py?name=world HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Connection: keep-alive\r\n"
        "\r\n"
    ),

    "POST CGI": (
        "POST /cgi-bin/cgi_testscript.py HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: 10\r\n"
        "Connection: keep-alive\r\n"
        "\r\n"
        "name=world"
    ),
}


def recv_response(sock):
    response = b""

    while True:
        try:
            chunk = sock.recv(4096)

            if not chunk:
                print("[SERVER CLOSED CONNECTION]")
                return response

            response += chunk

            # This is intentionally simple: stop once we have received
            # something. For a CGI/HTTP server, you may want proper
            # Content-Length/chunked-response parsing instead.
            break

        except socket.timeout:
            break

    return response


with socket.create_connection((HOST, PORT)) as sock:
    sock.settimeout(10.0)

    print(f"Connected to {HOST}:{PORT}")

    while True:
        print("\nAvailable tests:")

        for i, name in enumerate(requests, 1):
            print(f"  {i}. {name}")

        print("  q. Quit")

        choice = input("\n> ").strip()

        if choice.lower() == "q":
            break

        try:
            request_name = list(requests)[int(choice) - 1]
        except (ValueError, IndexError):
            print("Invalid choice")
            continue

        request = requests[request_name]

        print(f"\n--- Sending {request_name} ---")
        print(request.replace("\r", "\\r").replace("\n", "\\n\n"))

        sock.sendall(request.encode())

        response = recv_response(sock)

        print(f"\n--- Received {len(response)} bytes ---")
        print(response.decode(errors="replace"))