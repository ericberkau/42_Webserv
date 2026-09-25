#!/usr/bin/env python3

import socket
import threading

HOST = "127.0.0.1"
PORT = 8080


def receiver(sock):
    while True:
        try:
            data = sock.recv(4096)

            if not data:
                print("\n[SERVER CLOSED CONNECTION]")
                break

            print("\n--- RESPONSE ---")
            print(data.decode(errors="replace"))
            print("----------------\n")

        except Exception as e:
            print(f"\n[RECV ERROR] {e}")
            break


with socket.create_connection((HOST, PORT)) as sock:

    print(f"Connected to {HOST}:{PORT}")

    recv_thread = threading.Thread(
        target=receiver,
        args=(sock,),
        daemon=True
    )
    recv_thread.start()

    while True:

        print(
            "\nPaste raw HTTP request(s).\n"
            "Finish input with a line containing only END.\n"
            "Type QUIT to close the client.\n"
        )

        lines = []

        while True:
            line = input()

            if line == "QUIT":
                print("Closing connection.")
                sock.close()
                raise SystemExit

            if line == "END":
                break

            lines.append(line)

        payload = "\n".join(lines)

        payload = payload.replace("\\r", "\r")
        payload = payload.replace("\\n", "\n")

        sock.sendall(payload.encode())

        print(f"\n[SENT {len(payload)} BYTES]")

    # GET /images/ HTTP/1.1\r\nHost: localhost\r\n\r\nGET / HTTP/1.1\r\nHost: localhost\r\n\r\n
