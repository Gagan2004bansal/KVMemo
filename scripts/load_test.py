import socket, time

HOST, PORT = "127.0.0.1", 8082
N = 10000

def send_cmd(sock, cmd):
    msg = cmd.encode()
    sock.sendall(len(msg).to_bytes(4, 'big') + msg)

start = time.perf_counter()
s = socket.create_connection((HOST, PORT))
for i in range(N):
    send_cmd(s, f"SET key{i} value{i}")
elapsed = (time.perf_counter() - start) * 1000
print(f"Insert {N} memos: {elapsed:.1f}ms  ({N/elapsed*1000:.0f} ops/sec)")
s.close()