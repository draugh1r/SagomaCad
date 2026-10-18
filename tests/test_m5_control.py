"""M5 regressions against the native app's authenticated loopback channel."""
import json
import math
from pathlib import Path
import socket
import subprocess
import sys
import tempfile
import time

with tempfile.TemporaryDirectory() as folder:
    root = Path(folder)
    token = root / "token"
    token.write_text("m5-control-test", encoding="utf-8")
    with socket.socket() as probe:
        probe.bind(("127.0.0.1", 0))
        port = probe.getsockname()[1]
    process = subprocess.Popen([sys.argv[1], "--control", str(port),
        "--control-token-file", str(token), "--automation-root", str(root),
        "--session-file", str(root / "session.json")], stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    try:
        deadline = time.monotonic() + 15
        while True:
            try:
                connection = socket.create_connection(("127.0.0.1", port), timeout=1)
                break
            except OSError:
                if process.poll() is not None or time.monotonic() > deadline:
                    raise RuntimeError("Native control did not start")
                time.sleep(.1)
        with connection:
            connection.settimeout(10)
            reader = connection.makefile("r", encoding="utf-8")
            counter = 0
            def call(method, params=None):
                global counter
                counter += 1
                connection.sendall((json.dumps({"id": counter, "method": method, "params": params or {}}) + "\n").encode())
                reply = json.loads(reader.readline())
                assert "result" in reply, reply
                return reply["result"]
            def command(name, **params):
                return call("engine.execute", {"command": name, "params": params})
            call("auth", {"token": "m5-control-test"})
            previous = command("preferences.get")["hide_sketch_after_feature"]
            try:
                command("preferences.set", hide_sketch_after_feature=False)
                sketch = command("sketch.create", plane="XY")["sketch"]
                rect = command("sketch.rect", width="20 mm", height="10 mm")
                circle = command("sketch.circle", x=40, y=5, diameter="6 mm")
                command("sketch.finish")
                command("feature.extrude", sketch=sketch, profiles=[rect["entities"]], distance="5 mm")
                source = command("sketch.inspect", sketch=sketch)
                assert source["visible"] is True and len(source["regions"]) == 2, source
                second = command("feature.extrude", sketch=sketch, profiles=[[circle["entity"]]], distance="8 mm")
                assert abs(command("model.mass_properties", body=second["body_id"])["volume_mm3"] - 72*math.pi) < 1e-5
                command("sketch.edit", sketch=sketch)
                for _ in range(10):
                    state = call("ui.inspect")
                    if state["sketch"]["dimension_rects"]:
                        break
                assert len(state["sketch"]["dimension_rects"]) == 3, state["sketch"]
                assert "live_fields" in state["sketch"] and "dimension_edit_rect" in state["sketch"]
                image = Path(call("ui.screenshot", {"path": "m5-quotes.png"})["path"])
                assert image.read_bytes()[:8] == b"\x89PNG\r\n\x1a\n"
                command("document.new")
                box = command("primitive.add", kind="box", dimensions={"width":"10 mm", "depth":"20 mm", "height":"30 mm"})
                face = {"body":box["body_id"], "name":box["feature_id"]+"/x_max"}
                assert command("feature.extrude.suggest", face=face, distance="7 mm")["operation"] == "join"
                result = command("feature.extrude", face=face, distance="7 mm")
                assert result["body_id"] == box["body_id"]
                mass = command("model.mass_properties", body=box["body_id"])
                assert abs(mass["volume_mm3"] - 10200) < 1e-5 and abs(mass["area_mm2"] - 2900) < 1e-5, mass
                assert len(command("document.inspect")["document"]["bodies"]) == 1
            finally:
                command("preferences.set", hide_sketch_after_feature=previous)
                call("ui.inspect")
    finally:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait(timeout=5)
print("M5 native control: preference, unused profile, dimension rectangles, PNG and automatic union OK")
