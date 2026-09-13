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