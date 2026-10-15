"""
LedgerOS Live Market Data Serial Bridge
Fetches cryptocurrency prices from Binance's public API and streams them
to the LedgerOS QEMU COM2 TCP serial endpoint.
"""

import time
import urllib.request
import json
import socket

QEMU_SERIAL_HOST = "127.0.0.1"
QEMU_SERIAL_PORT = 4444
def get_binance_price(symbol="BTCUSDT"):
    try:
        url = f"https://api.binance.com/api/v3/ticker/price?symbol={symbol}"
        req = urllib.request.Request(url, headers={"User-Agent": "LedgerOS-Bridge"})
        with urllib.request.urlopen(req, timeout=3) as resp:
            data = json.loads(resp.read().decode())
            return int(float(data["price"]))
    except Exception:
        return None

def connect_serial():
    try:
        sock = socket.create_connection((QEMU_SERIAL_HOST, QEMU_SERIAL_PORT), timeout=3)
        sock.settimeout(3)
        print("[BRIDGE] Connected to LedgerOS COM2.", flush=True)
        return sock
    except OSError as e:
        print(f"[BRIDGE] Waiting for QEMU serial socket: {e}", flush=True)
        return None

def main():
    print(f"[BRIDGE] Binance prices -> LedgerOS COM2 ({QEMU_SERIAL_HOST}:{QEMU_SERIAL_PORT})", flush=True)
    sock = None
    next_connect_attempt = 0.0

    idx = 0
    symbols = ["BTCUSDT", "ETHUSDT"]
    labels = ["BTC", "ETH"]

    while True:
        now = time.monotonic()
        if sock is None and now >= next_connect_attempt:
            sock = connect_serial()
            next_connect_attempt = now + 5

        sym = symbols[idx % len(symbols)]
        label = labels[idx % len(labels)]
        price = get_binance_price(sym)
        if price is not None:
            cmd = f"msg {label} {price}\n"
            print(f"[API -> LedgerOS] {cmd.strip()}", flush=True)
            if sock is not None:
                try:
                    sock.sendall(cmd.encode("ascii"))
                except OSError as e:
                    print(f"[BRIDGE] Connection to QEMU lost: {e}", flush=True)
                    sock.close()
                    sock = None
                    next_connect_attempt = time.monotonic() + 1
        time.sleep(2)
        idx += 1

if __name__ == "__main__":
    main()
