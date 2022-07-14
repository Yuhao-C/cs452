import sys
import time
from collections import deque

# python3 -u ./tc2_demo.py > /dev/ttyS0

cmdBuf = deque([])


def p(s: str):
    print(s, file=sys.stderr)
    cmdBuf.append(s)


def flush():
    print("execute?", file=sys.stderr)
    input()
    while cmdBuf:
        print(cmdBuf.popleft())


p("loc 1 A8 0 b")
p("loc 24 A1 0 b")
p("loc 78 A10 0 f")
p("loc 58 A13 0 f")
flush()
p("route 24 E1 0")
p("route 78 D1 0")
p("route 58 B2 0")
flush()
p("route 24 D11 0")
p("route 78 B6 0")
flush()
p("route 78 A16 0")
p("route 58 A12 0")
p("route 24 C16 0")
flush()
p("route 58 B13 0")
p("route 78 C2 0")
p("route 24 E1 0")
p("route 1 D1 0")
flush()
