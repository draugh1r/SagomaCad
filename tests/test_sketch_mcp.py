"""M3 acceptance entirely through MCP, plus CLI parity for inspection."""
import json
import subprocess
import sys
import tempfile
import re
from pathlib import Path
from test_mcp_integration import Client, png

cli = sys.argv[1]
client = Client([cli, "mcp", "--headless"])
try:
    def command(command_id, **args):
        return client.ok("command_run", {"command": command_id, "params": args})
    assert command("preferences.get")["hide_sketch_after_feature"] is False
    assert command("preferences.set", hide_sketch_after_feature=True)["hide_sketch_after_feature"] is True
    assert command("preferences.set", hide_sketch_after_feature=False)["hide_sketch_after_feature"] is False
    command("param.set", name="foro", expression="10 mm")
    sketch = command("sketch.create", plane="XY")["sketch"]
    rectangle = command("sketch.rect", width="60 mm", height="40 mm", centered=True)
    circle = command("sketch.circle", diameter="foro")
    result = client.ok("sketch_inspect", {"sketch": sketch})
    assert result["state"] == "constrained" and result["degrees_of_freedom"] == 0, result
    assert len(result["entities"]) == 5 and len(result["profiles"]) == 2
    assert abs(result["points"][0]["x"] + 30) < 1e-5
    assert abs(result["points"][0]["y"] + 20) < 1e-5
    command("param.set", name="foro", expression="16 mm")
    assert abs(client.ok("sketch_inspect")["entities"][-1]["radius"] - 8) < 1e-5
    client.ok("undo")
    assert abs(client.ok("sketch_inspect")["entities"][-1]["radius"] - 5) < 1e-5
    client.ok("redo")
    assert abs(client.ok("sketch_inspect")["entities"][-1]["radius"] - 8) < 1e-5
    command("sketch.finish")
    command("sketch.visibility", sketch=sketch, visible=False)
    assert client.ok("sketch_inspect", {"sketch": sketch})["visible"] is False
    client.ok("undo")
    assert client.ok("sketch_inspect", {"sketch": sketch})["visibility"] is None
    command("sketch.visibility", sketch=sketch, visible=True)
    command("sketch.edit", sketch=sketch)
    dimension = result["dimensions"][0]["id"]
    command("sketch.dimension.set", dimension=dimension, expression="70 mm")
    assert client.ok("sketch_inspect")["state"] == "constrained"
    client.ok("undo")
    # Exercise the other command paths through MCP on a separate sketch.
    other = command("sketch.create", plane="XZ")["sketch"]
    line = command("sketch.line", x1=2, y1=3, x2=12, y2=3)
    command("sketch.point.move", point=line["points"][0], x=4, y=6)
    command("sketch.constraint.add", kind="horizontal", refs=[line["entity"]])
    command("sketch.dimension.add", kind="length", refs=[line["entity"]], expression="10 mm")
    command("sketch.arc", radius="8 mm", start_angle="0 deg", end_angle="90 deg")
    assert client.ok("sketch_inspect", {"sketch": other})["plane"] == "XZ"
    command("sketch.edit", sketch=sketch)
    assert abs(client.ok("sketch_inspect")["entities"][-1]["radius"] - 8) < 1e-5
    loose = command("sketch.create", plane="YZ")["sketch"]
    command("sketch.rect", width="20 mm", height="15 mm", driving_width=False, driving_height=False)
    command("sketch.circle", diameter="10 mm", driving_diameter=False)
    free = client.ok("sketch_inspect", {"sketch": loose})
    assert free["dimensions"] == [] and free["degrees_of_freedom"] == 3, free
    command("sketch.edit", sketch=sketch)
    png(client.call("render_view", {"view": "top", "size": "640x480"}), 640, 480)
    assert client.call("command_run", {"command": "sketch.dimension.set", "params": {
        "dimension": "missing", "expression": "20 mm"}})["isError"]
    conflict = command("sketch.dimension.add", kind="length", refs=[rectangle["entities"][0]], expression="100 mm")
    assert conflict["solver"]["state"] == "conflict" and conflict["solver"]["conflicting"]
    client.ok("undo")
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "sketch.sagomacad"
        # command_run restricts file access; save through the root-aware document tool.
        document = client.ok("document_inspect")["document"]
        path.write_text(json.dumps(document), encoding="utf-8")
        reply = subprocess.run([cli, "call", "engine.execute", json.dumps({
            "command": "sketch.inspect", "params": {"sketch": sketch}}),
            "--headless", str(path)], capture_output=True, text=True, encoding="utf-8")
        assert reply.returncode == 0, reply.stderr + reply.stdout
        inspected = json.loads(reply.stdout)
        assert inspected["result"]["state"] == "constrained", inspected
        assert inspected["result"]["visibility"] is True, inspected
        hidden = subprocess.run([cli, "call", "engine.execute", json.dumps({
            "command": "sketch.visibility", "params": {"sketch": sketch, "visible": False}}),
            "--headless", str(path)], capture_output=True, text=True, encoding="utf-8")
        assert hidden.returncode == 0 and json.loads(hidden.stdout)["result"]["visible"] is False, hidden.stdout + hidden.stderr
        guide = (Path(__file__).resolve().parents[1] / "docs/sketches.md").read_text(encoding="utf-8")
        script = re.search(r"```json\n(\[\n.*?\n\])\n```", guide, re.S).group(1)
        script_path = Path(tmp) / "session.json"
        script_path.write_text(script, encoding="utf-8")
        cli_path = Path(tmp) / "from-cli.sagomacad"
        reply = subprocess.run([cli, "run", "--script-file", str(script_path), "--headless", str(cli_path)],
                               capture_output=True, text=True, encoding="utf-8")
        assert reply.returncode == 0, reply.stderr
        assert json.loads(reply.stdout)["results"][-1]["result"]["state"] == "constrained"
        assert cli_path.is_file()
        for draft in ({"first": 42}, {"x": 1e300}, {"tool": "missing"}):
            bad = subprocess.run([cli, "snapshot", "--out", str(Path(tmp) / "bad.png"),
                                  "--script", json.dumps([["ui.set", {"sketch_draft": draft}]])],
                                 capture_output=True)
            assert bad.returncode == 2, bad.stderr
finally:
    client.close()
print("MCP: centered 60x40 rectangle + parametric circle, conflict, undo, CLI parity OK")
