"""Exercise authentication, inspection and PNG capture over loopback."""
import json
import socket
import struct
import subprocess
import sys
import tempfile
import time
from pathlib import Path

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    token_file = root / "token"
    token_file.write_text("m0-test-token", encoding="utf-8")
    with socket.socket() as probe:
        probe.bind(("127.0.0.1", 0))
        port = probe.getsockname()[1]
    process = subprocess.Popen([sys.argv[1], "--control", str(port),
                                "--control-token-file", str(token_file),
                                "--automation-root", str(root)],
                               stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    try:
        deadline = time.monotonic() + 15
        while True:
            try:
                client = socket.create_connection(("127.0.0.1", port), timeout=1)
                break
            except OSError:
                if process.poll() is not None:
                    raise RuntimeError(process.stderr.read().decode(errors="replace"))
                if time.monotonic() > deadline:
                    raise TimeoutError("control server did not start")
                time.sleep(.1)
        with client:
            client.settimeout(10)
            reader = client.makefile("r", encoding="utf-8")
            def call(identifier, method, params=None):
                request = {"id": identifier, "method": method, "params": params or {}}
                client.sendall((json.dumps(request) + "\n").encode())
                return json.loads(reader.readline())
            assert "error" in call(1, "ui.inspect")
            assert "error" in call(2, "auth", {"token": "wrong"})
            assert call(3, "auth", {"token": "m0-test-token"})["result"]["authenticated"]
            state = call(4, "ui.inspect")["result"]
            assert state["docking"] and state["viewport"]["grid"]
            assert state["fixed_layout"] and not state["tabs_visible"]
            assert not state["viewport"]["title_visible"]
            assert "error" in call(5, "ui.screenshot", {"path": "../escape.png"})
            path = Path(call(6, "ui.screenshot", {"path": "control.png"})["result"]["path"])
            assert path.is_file()
            header = path.read_bytes()[:24]
            assert header[:8] == b"\x89PNG\r\n\x1a\n"
            width, height = struct.unpack(">II", header[16:24])
            # SDL reports framebuffer pixels, which can differ from window points on Retina.
            assert 320 <= width <= 8192 and 240 <= height <= 8192
    finally:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait(timeout=5)
