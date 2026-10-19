"""Exercise authentication, inspection and PNG capture over loopback."""
import json
import os
import socket
import struct
import subprocess
import sys
import tempfile
import time
from pathlib import Path

def main(executable):
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        token_file = root / "token"
        token_file.write_text("m0-test-token", encoding="utf-8")
        with socket.socket() as probe:
            probe.bind(("127.0.0.1", 0))
            port = probe.getsockname()[1]
        process = subprocess.Popen([executable, "--control", str(port),
                                    "--control-token-file", str(token_file),
                                    "--automation-root", str(root), "--settings-file", str(root / "layout.json"), "--session-file", str(root / "session.json")],
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
                assert not state["docking"] and state["viewport"]["grid"]
                assert state["layout_engine"] == "clay"
                assert {r["id"] for r in state["shell_rects"]} >= {"topbar", "ribbon", "browser", "viewport", "properties", "timeline", "contextual"}
                assert all(r["width"] >= 0 and r["height"] >= 0 for r in state["shell_rects"])
                assert state["fixed_layout"] and not state["tabs_visible"]
                assert not state["viewport"]["title_visible"]
                handles = state["handles"]
                assert {"menu:CREA", "cube:home", "cube:top-front", "row:Origine", "fold:Origine"} <= handles.keys()
                assert "toolbar:primitive.add" not in handles
                assert "result" in call(18,"ui.click",{"handle":"menu:CREA"})
                assert "toolbar:primitive.add" in call(19,"ui.inspect")["result"]["handles"]
                point = call(20,"ui.at",{"handle":"toolbar:primitive.add"})["result"]
                assert point["x"] > 0 and point["y"] > 0
                assert "error" in call(21,"ui.at",{"handle":"missing"})
                assert "error" in call(22,"ui.click",{"handle":"bodies"})
                assert "result" in call(23,"ui.click",{"handle":"toolbar:primitive.add"})
                assert "field:Larghezza" in call(24,"ui.inspect")["result"]["handles"]
                assert "result" in call(25,"ui.key",{"key":"Escape"})
                assert "field:Larghezza" not in call(26,"ui.inspect")["result"]["handles"]
                assert "result" in call(27,"ui.resize",{"width":1280,"height":720})
                assert call(28,"ui.inspect")["result"]["width"] == 1280

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


def test_handles():
    executable = os.environ.get("SAGOMACAD_APP") or str(Path(__file__).resolve().parents[1] / "build/apps/sagomacad/sagomacad.exe")
    assert Path(executable).is_file(), "Set SAGOMACAD_APP to the built app executable"
    main(executable)


if __name__ == "__main__":
    main(sys.argv[1])
