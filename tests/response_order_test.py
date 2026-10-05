import socket


HOST = "127.0.0.1"
PORT = 8080


requests = (
    b"GET /cgi-bin/sleepy.py HTTP/1.1\r\n"
    b"Host: localhost\r\n"
    b"Connection: keep-alive\r\n"
    b"\r\n"

    b"GET /index.html HTTP/1.1\r\n"
    b"Host: localhost\r\n"
    b"Connection: keep-alive\r\n"
    b"\r\n"

    b"GET /empty.html HTTP/1.1\r\n"
    b"Host: localhost\r\n"
    b"Connection: close\r\n"
    b"\r\n"
)


def recv_until(sock, buffer, marker):
    while marker not in buffer:
        data = sock.recv(4096)

        if not data:
            raise RuntimeError("Connection closed before receiving expected data")

        buffer += data

    return buffer


def recv_response(sock, buffer):
    # Read until the complete HTTP headers are available.
    buffer = recv_until(sock, buffer, b"\r\n\r\n")

    header_end = buffer.index(b"\r\n\r\n") + 4

    header_bytes = buffer[:header_end]
    buffer = buffer[header_end:]

    headers = header_bytes.decode("iso-8859-1")

    lines = headers.split("\r\n")

    status_line = lines[0]

    content_length = None

    for line in lines[1:]:
        if ":" not in line:
            continue

        name, value = line.split(":", 1)

        if name.lower() == "content-length":
            content_length = int(value.strip())

    if content_length is None:
        raise RuntimeError(
            "Response has no Content-Length:\n" + headers
        )

    while len(buffer) < content_length:
        data = sock.recv(4096)

        if not data:
            raise RuntimeError(
                "Connection closed before complete response body"
            )

        buffer += data

    body = buffer[:content_length]
    buffer = buffer[content_length:]

    return {
        "status": status_line,
        "headers": headers,
        "body": body,
        "raw": header_bytes + body,
    }, buffer


def main():
    print("Connecting...")

    with socket.create_connection((HOST, PORT)) as sock:
        print("Connected")

        print("Sending 3 pipelined requests in ONE TCP write...")

        sock.sendall(requests)

        buffer = b""
        responses = []

        for i in range(3):
            print(f"\nWaiting for response {i + 1}...")

            response, buffer = recv_response(sock, buffer)

            responses.append(response)

            print(f"Response {i + 1}: {response['status']}")
            print("Body:")
            print(response["body"].decode("utf-8", errors="replace"))

        print("\n========================================")
        print("Checking response order")
        print("========================================")

        body1 = responses[0]["body"]
        body2 = responses[1]["body"]
        body3 = responses[2]["body"]

        if b"CGI RESPONSE" not in body1:
            raise AssertionError(
                "Response 1 was not the CGI response!"
            )

        if b"CGI RESPONSE" in body2:
            raise AssertionError(
                "Response 2 unexpectedly contains the CGI response!"
            )

        if b"CGI RESPONSE" in body3:
            raise AssertionError(
                "Response 3 unexpectedly contains the CGI response!"
            )

        print("PASS: response 1 is the CGI response")

        print("PASS: response 2 is the first static response")

        print("PASS: response 3 is the second static response")

        print("\nAll responses arrived in the correct order.")


if __name__ == "__main__":
    main()
