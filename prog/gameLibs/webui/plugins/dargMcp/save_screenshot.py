"""Call the daRg MCP make_screenshot tool and save the result to a JPEG file.

Uses a raw socket to send the entire HTTP request in a single send() call.
Python's urllib sends headers and body separately, which triggers a TCP RST
from the webui server (it does a single recvfrom, so the unsent body remains
unread in the socket buffer, causing closesocket to RST).
"""

import json
import base64
import socket
import sys

HOST = "127.0.0.1"
PORT = 23456
PATH = "/darg-llm-mcp"
DEFAULT_OUTPUT = "screenshot.png"


def mcp_call(method, params=None):
    body = json.dumps({
        "jsonrpc": "2.0",
        "id": 1,
        "method": method,
        "params": params or {},
    }).encode()

    request = (
        f"POST {PATH} HTTP/1.0\r\n"
        f"Host: {HOST}:{PORT}\r\n"
        f"Content-Type: application/json\r\n"
        f"Content-Length: {len(body)}\r\n"
        f"\r\n"
    ).encode() + body

    with socket.create_connection((HOST, PORT), timeout=30) as sock:
        sock.sendall(request)

        # Read entire response
        chunks = []
        while True:
            chunk = sock.recv(65536)
            if not chunk:
                break
            chunks.append(chunk)

    raw = b"".join(chunks)
    # Split HTTP header from body
    header_end = raw.find(b"\r\n\r\n")
    if header_end < 0:
        raise RuntimeError("Malformed HTTP response")
    return json.loads(raw[header_end + 4:])


def main():
    output = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_OUTPUT

    data = mcp_call("tools/call", {"name": "make_screenshot", "arguments": {}})

    for item in data["result"]["content"]:
        if item["type"] == "text":
            print(item["text"])
        elif item["type"] == "image":
            with open(output, "wb") as f:
                f.write(base64.b64decode(item["data"]))
            print(f"Saved to {output}")


if __name__ == "__main__":
    main()
