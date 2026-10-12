"""
LedgerOS Live Market Data Serial Bridge
Fetches real cryptocurrency prices from Binance Public API and streams them
into the LedgerOS QEMU serial port (COM1 / UART 16550) via TCP socket or pipe.
"""

import time
import urllib.request
import json
import socket
import sys

QEMU_SERIAL_HOST = "127.0.0.1"
QEMU_SERIAL_PORT = 4444
TICKERS = ["BTCUSDT", "ETHUSDT", "SOLUSDT", "NVDA", "AAPL"]

def get_binance_price(symbol="BTCUSDT"):
    try:
        url = f"https://api.binance.com/api/v3/ticker/price?symbol={symbol}"
        req = urllib.request.Request(url, headers={"User-Agent": "LedgerOS-Bridge"})
        with urllib.request.urlopen(req, timeout=3) as resp:
            data = json.loads(resp.read().decode())
            return int(float(data["price"]))
    except Exception:
        return None

def main():
    print(f"[BRIDGE] Connecting to LedgerOS QEMU Serial at {QEMU_SERIAL_HOST}:{QEMU_SERIAL_PORT}...")
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((QEMU_SERIAL_HOST, QEMU_SERIAL_PORT))
        print("[BRIDGE] Connected! Streaming live market ticks to LedgerOS...")
    except Exception as e:
        print(f"[BRIDGE INFO] Running in standalone demonstration mode (QEMU socket not bound: {e})")
        sock = None

    idx = 0
    symbols = ["BTCUSDT", "ETHUSDT"]
    labels = ["BTC", "ETH"]

    while True:
        sym = symbols[idx % len(symbols)]
        label = labels[idx % len(labels)]
        price = get_binance_price(sym)
        if price is not None:
            cmd = f"msg {label} {price}\n"
            print(f"[API -> LedgerOS] {cmd.strip()}")
            if sock:
                try:
                    sock.sendall(cmd.encode("ascii"))
                except Exception:
                    print("[BRIDGE] Connection to QEMU lost.")
                    break
        time.sleep(2)
        idx += 1

if __name__ == "__main__":
    main()
