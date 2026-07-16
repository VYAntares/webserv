#!/usr/bin/env python3
"""Stress test webserv : N requêtes concurrentes, mélange de cibles.

Vérifie l'exigence du sujet : « Stress test your server to ensure it remains
available at all times. »

Usage : python3 tests/stress_test.py [host:port] [nb_requetes] [concurrence]
"""
import sys
import time
import http.client
import threading

HOST = sys.argv[1] if len(sys.argv) > 1 else "localhost:8080"
TOTAL = int(sys.argv[2]) if len(sys.argv) > 2 else 500
CONC = int(sys.argv[3]) if len(sys.argv) > 3 else 50

TARGETS = [
    ("GET", "/default/index.html", None, (200,)),
    ("GET", "/files/hello.txt", None, (200,)),
    ("GET", "/files/", None, (200,)),
    ("GET", "/nexiste-pas", None, (404,)),
    ("GET", "/cgi-bin/hello.py?stress=1", None, (200,)),
    ("POST", "/cgi-bin/post_echo.py", b"a" * 2048, (200,)),
]

lock = threading.Lock()
stats = {"done": 0, "ok": 0, "bad": 0, "err": 0, "lat": []}


def worker(n):
    host, port = HOST.split(":")
    for i in range(n):
        method, path, body, expect = TARGETS[(i * 7) % len(TARGETS)]
        t0 = time.time()
        try:
            conn = http.client.HTTPConnection(host, int(port), timeout=15)
            conn.request(method, path, body=body)
            resp = conn.getresponse()
            resp.read()
            conn.close()
            lat = (time.time() - t0) * 1000
            with lock:
                stats["lat"].append(lat)
                if resp.status in expect:
                    stats["ok"] += 1
                else:
                    stats["bad"] += 1
        except Exception:
            with lock:
                stats["err"] += 1
        with lock:
            stats["done"] += 1


def main():
    per = TOTAL // CONC
    print("Stress %s : %d requêtes, %d threads (%d req/thread)"
          % (HOST, per * CONC, CONC, per))
    t0 = time.time()
    threads = [threading.Thread(target=worker, args=(per,)) for _ in range(CONC)]
    for t in threads:
        t.start()
    for t in threads:
        t.join()
    dur = time.time() - t0

    lats = sorted(stats["lat"]) or [0]
    print()
    print("Réussies       : %d" % stats["ok"])
    print("Mauvais code   : %d" % stats["bad"])
    print("Erreurs réseau : %d" % stats["err"])
    print("Durée          : %.1f s (%.0f req/s)" % (dur, stats["done"] / dur))
    print("Latence        : moy %.1f ms / p95 %.1f ms / max %.1f ms"
          % (sum(lats) / len(lats), lats[int(len(lats) * .95) - 1], lats[-1]))

    # le serveur est-il toujours vivant ?
    host, port = HOST.split(":")
    try:
        conn = http.client.HTTPConnection(host, int(port), timeout=5)
        conn.request("GET", "/default/index.html")
        alive = conn.getresponse().status == 200
        conn.close()
    except Exception:
        alive = False
    print("Disponibilité  : %s" % ("OK, le serveur répond toujours" if alive
                                   else "KO, le serveur ne répond plus !"))
    sys.exit(0 if (alive and stats["err"] == 0 and stats["bad"] == 0) else 1)


if __name__ == "__main__":
    main()
