"""Drive modeling through MCP only, in headless and bridge modes."""
import base64
import json
import math
import os
from pathlib import Path
import socket
import struct
import subprocess
import sys
import tempfile
import time


class Client:
    def __init__(self, command):
        self.process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE, text=True, encoding="utf-8", bufsize=1)
        self.number = 0

    def call(self, tool, arguments=None):
        self.number += 1
        request = {"jsonrpc": "2.0", "id": self.number, "method": "tools/call",
                   "params": {"name": tool, "arguments": arguments or {}}}
        self.process.stdin.write(json.dumps(request) + "\n")
        self.process.stdin.flush()
        response = json.loads(self.process.stdout.readline())
        assert response["id"] == self.number and "result" in response, response
        return response["result"]

    def ok(self, tool, arguments=None):
        result = self.call(tool, arguments)
        assert not result.get("isError"), result
        return result["structuredContent"]

    def close(self):
        self.process.stdin.close()
        assert self.process.wait(timeout=10) == 0, self.process.stderr.read()


def png(content, width=None, height=None):
    image = next(item for item in content["content"] if item["type"] == "image")
    assert image["mimeType"] == "image/png"
    data = base64.b64decode(image["data"], validate=True)
    assert data[:8] == b"\x89PNG\r\n\x1a\n"
    actual = struct.unpack(">II", data[16:24])
    if width is not None:
        assert actual == (width, height), actual
    return data


def stl_volume(path):
    data = path.read_bytes()
    count = struct.unpack_from("<I", data, 80)[0]
    assert len(data) == 84 + 50 * count and count >= 12
    volume = 0.0
    for index in range(count):
        floats = struct.unpack_from("<12f", data, 84 + 50 * index)
        a, b, c = (floats[3:6], floats[6:9], floats[9:12])
        volume += (a[0] * (b[1] * c[2] - b[2] * c[1]) +
                   a[1] * (b[2] * c[0] - b[0] * c[2]) +
                   a[2] * (b[0] * c[1] - b[1] * c[0])) / 6
    return abs(volume)


def headless(cli, root):
    client = Client([cli, "mcp", "--headless"])
    document_path = root / f"mcp-integration-{os.getpid()}.sagomacad"
    export_name = f"mcp-integration-{os.getpid()}"
    export_path = root / "export" / (export_name + ".stl")
    try:
        client.ok("command_run", {"command": "param.set", "params": {"name": "larghezza", "expression": "40 mm"}})
        box = client.ok("command_run", {"command": "primitive.add", "params": {
            "kind": "box", "dimensions": {"width": "larghezza", "depth": "larghezza", "height": "20 mm"}}})
        body = box["body_id"]
        client.ok("command_run", {"command": "primitive.add", "params": {
            "kind": "cylinder", "operation": "subtract", "body": body,
            "dimensions": {"radius": "5 mm", "height": "20 mm"},
            "position": {"x": "larghezza / 2", "y": "larghezza / 2", "z": "0 mm"}}})
        mass = client.ok("mass_properties", {"body": body})
        assert abs(mass["volume_mm3"] - (32000 - math.pi * 500)) < 0.001, mass
        assert mass["bbox_mm"]["max"]["x"] > 39.99
        before_edit = client.ok("document_inspect")
        failed_edit = client.call("command_run", {"command": "primitive.edit", "params": {
            "feature_id": box["feature_id"], "dimensions": {"width": "60 mm"}, "name": 42}})
        assert failed_edit["isError"]
        assert client.ok("document_inspect") == before_edit
        client.ok("command_run", {"command": "param.set", "params": {
            "old_name": "larghezza", "name": "lato", "expression": "40 mm"}})
        assert abs(client.ok("mass_properties", {"body": body})["volume_mm3"] - mass["volume_mm3"]) < 0.001
        assert client.call("command_run", {"command": "param.delete", "params": {"name": "lato"}})["isError"]
        client.ok("undo")
        client.ok("redo")
        client.ok("command_run", {"command": "param.set", "params": {
            "old_name": "lato", "name": "larghezza", "expression": "40 mm"}})
        for view in ("iso", "top", "front", "right"):
            png(client.call("render_view", {"view": view, "size": "640x480"}), 640, 480)
        before = client.ok("document_inspect")["history"]["steps"]
        client.ok("batch_run", {"steps": [
            {"command": "param.set", "params": {"name": "a", "expression": "2 mm"}},
            {"command": "param.set", "params": {"name": "b", "expression": "a * 2"}}]})
        assert client.ok("document_inspect")["history"]["steps"] == before + 1
        client.ok("undo")
        assert len(client.ok("document_inspect")["document"]["parameters"]) == 1
        client.ok("redo")
        assert len(client.ok("document_inspect")["document"]["parameters"]) == 3
        denied = client.call("document_save", {"path": str(document_path)})
        assert denied["isError"] and denied["structuredContent"]["code"] == "mcp.path_denied"
        saved = client.ok("document_save", {"path": document_path.name})
        assert Path(saved["path"]).is_file()
        assert client.call("document_save", {"path": document_path.name})["structuredContent"]["code"] == "mcp.overwrite_required"
        client.ok("document_save", {"path": document_path.name, "overwrite": True})
        client.ok("command_run", {"command": "document.new"})
        client.ok("document_open", {"path": document_path.name})
        assert len(client.ok("document_inspect")["document"]["bodies"]) == 1
        exported = client.ok("export", {"format": "stl", "path": export_name, "body": body, "tolerance": 0.001})
        assert Path(exported["path"]) == export_path and export_path.is_file()
        assert export_path.suffix == ".stl"
        measured_stl = stl_volume(export_path)
        assert abs(measured_stl - (32000 - math.pi * 500)) < 2.0, measured_stl
        assert client.call("export", {"format": "stl", "path": export_name})["structuredContent"]["code"] == "mcp.overwrite_required"
        assert client.call("ui_screenshot")["structuredContent"]["code"] == "mcp.bridge_required"
        views = []
        for order in ((0, 30), (30, 0)):
            client.ok("command_run", {"command": "document.new"})
            for z in order:
                client.ok("command_run", {"command": "primitive.add", "params": {
                    "kind": "box", "dimensions": {"width": "40 mm", "depth": "40 mm", "height": "20 mm"},
                    "position": {"z": f"{z} mm"}}})
            views.append(png(client.call("render_view", {"view": "iso", "size": "640x480"})))
        assert views[0] == views[1], "viewport occlusion depends on body creation order"
    finally:
        client.close()
        document_path.unlink(missing_ok=True)
        export_path.unlink(missing_ok=True)


def bridge(app, cli, root):
    with tempfile.TemporaryDirectory() as temporary:
        temporary = Path(temporary)
        token = temporary / "token"
        token.write_text("mcp-bridge-test-token", encoding="utf-8")
        with socket.socket() as probe:
            probe.bind(("127.0.0.1", 0))
            port = probe.getsockname()[1]
        application = subprocess.Popen([app, "--control", str(port), "--control-token-file", str(token),
                                        "--automation-root", str(temporary),
                                        "--session-file", str(temporary / "session.json")],
                                       stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        client = None
        try:
            deadline = time.monotonic() + 15
            while True:
                try:
                    with socket.create_connection(("127.0.0.1", port), timeout=1):
                        break
                except OSError:
                    if application.poll() is not None:
                        raise RuntimeError(application.stderr.read().decode(errors="replace"))
                    if time.monotonic() > deadline:
                        raise TimeoutError("app control not ready")
                    time.sleep(0.1)
            client = Client([cli, "mcp", "--bridge", str(port), "--token-file", str(token),
                             "--allow-root", str(root)])
            client.ok("command_run", {"command": "primitive.add", "params": {
                "kind": "box", "dimensions": {"width": "40 mm", "depth": "40 mm", "height": "20 mm"}}})
            assert len(client.ok("document_inspect")["document"]["bodies"]) == 1
            png(client.call("render_view", {"view": "iso", "size": "640x480"}), 640, 480)
            png(client.call("ui_screenshot"))
            def command(name, **params):
                return client.ok("command_run", {"command": name, "params": params})
            command("document.new")
            source = command("sketch.create", plane="XY")["sketch"]
            rectangle = command("sketch.rect", sketch=source, width="20 mm", height="10 mm")
            command("sketch.finish", sketch=source)
            feature = command("feature.extrude", sketch=source, profiles=[rectangle["entities"]], distance="10 mm")
            body = feature["body_id"]
            assert abs(client.ok("mass_properties", {"body": body})["volume_mm3"] - 2000) < 1e-5
            assert client.ok("feature_get", {"feature_id": feature["feature_id"]})["applied"]
            command("feature.edit", feature_id=feature["feature_id"], params={"distance": "15 mm"})
            assert abs(client.ok("mass_properties", {"body": body})["volume_mm3"] - 3000) < 1e-5
            command("timeline.rollback", index=1)
            assert not client.ok("feature_get", {"feature_id": feature["feature_id"]})["applied"]
            command("timeline.rollback", index=2)
            command("feature.suppress", feature_id=feature["feature_id"], suppressed=True)
            assert client.ok("feature_get", {"feature_id": feature["feature_id"]})["suppressed"]
            command("feature.suppress", feature_id=feature["feature_id"], suppressed=False)
            source = command("sketch.create", plane="XZ")["sketch"]
            rectangle = command("sketch.rect", sketch=source, x=5, width="10 mm", height="10 mm")
            command("sketch.finish", sketch=source)
            ring = command("feature.revolve", sketch=source, profiles=[rectangle["entities"]], axis="Z", type="full")
            assert abs(client.ok("mass_properties", {"body": ring["body_id"]})["volume_mm3"] - 2000*math.pi) < 1e-4
        finally:
            if client:
                client.close()
            application.terminate()
            try:
                application.wait(timeout=5)
            except subprocess.TimeoutExpired:
                application.kill()
                application.wait(timeout=5)


if __name__ == "__main__":
    mode, app, cli, root = sys.argv[1:]
    if mode == "headless":
        headless(cli, Path(root))
    else:
        bridge(app, cli, Path(root))
