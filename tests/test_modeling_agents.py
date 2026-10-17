"""Execute every published modeling-guide flow through real MCP and the CLI."""
import json
import math
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
from test_mcp_integration import png

cli = str(Path(sys.argv[1]).resolve())
root = Path(__file__).resolve().parents[1]
evidence = Path(sys.argv[2]).resolve()
evidence.mkdir(parents=True, exist_ok=True)
guide = (root / "docs/modeling-guide.md").read_text(encoding="utf-8")
blocks = re.findall(r"```python\n(.*?)\n```", guide, re.S)
assert len(blocks) == 6
with tempfile.TemporaryDirectory() as temporary:
    work = Path(temporary)
    scope = {"CLI": cli, "WORK": work}
    try:
        exec(compile(blocks[0], "modeling-guide-client", "exec"), scope)
        rpc, tool, command, region = (scope[name] for name in ("rpc", "tool", "command", "region"))
        served = rpc("resources/read", {"uri": "sagomacad://docs/modeling-guide"})["contents"][0]["text"]
        assert served.replace("\r\n", "\n") == guide, "embedded guide differs from the executable source"
        catalog = rpc("tools/list")["tools"]
        assert next(t for t in catalog if t["name"] == "feature_get")["inputSchema"]["required"] == ["feature_id"]
        for bad in ({}, {"feature_id": 3}, {"feature_id": "missing", "extra": True}):
            failure = rpc("tools/call", {"name": "feature_get", "arguments": bad})
            assert failure["isError"] and failure["structuredContent"]["code"] == "mcp.invalid_arguments", failure
        commands = {row["id"] for row in tool("commands_list")["commands"]}
        assert {"feature.extrude", "feature.revolve", "feature.edit", "feature.suppress", "timeline.rollback", "feature.get"} <= commands
        for index, code in enumerate(blocks[1:-1]):
            exec(compile(code, f"modeling-guide-flow-{index}", "exec"), scope)
        for artifact in ("m5-acceptance.sagomacad", "m5-labels.png"):
            shutil.copyfile(work / artifact, evidence / artifact)
        body = scope["bracket_body"]
        scope["check_volume"](body, 30000 - 250*math.pi)
        tool("document_open", {"path": "staffa-L.sagomacad"})
        measured = tool("mass_properties", {"body": body})
        assert abs(measured["volume_mm3"] - (22500 - 160*math.pi)) < 1e-4
        assert len(tool("document_inspect")["document"]["timeline"]) == 8
        shutil.copyfile(work / "staffa-L.sagomacad", evidence / "staffa-L.sagomacad")
        image = rpc("tools/call", {"name": "render_view", "arguments": {"view": "iso", "size": "960x720"}})
        (evidence / "staffa-L.png").write_bytes(png(image, 960, 720))
        (evidence / "mass-properties.json").write_text(json.dumps(measured, indent=2) + "\n", encoding="utf-8")

        # Revolve around an actual sketch line over MCP, then retain that document for CLI parity.
        source = command("sketch.create", plane="XZ")["sketch"]
        rectangle = command("sketch.rect", sketch=source, x=5, width="10 mm", height="10 mm")
        axis = command("sketch.line", sketch=source, x1=0, y1=0, x2=0, y2=30, construction=True)["entity"]
        profile = region(source, rectangle["entities"])
        command("sketch.finish", sketch=source)
        ring = command("feature.revolve", sketch=source, profiles=[profile], axis=axis, type="full")
        scope["check_volume"](ring["body_id"], 2000*math.pi)
        command("feature.edit", feature_id=ring["feature_id"], params={"type": "angle", "angle": "180 deg"})
        scope["check_volume"](ring["body_id"], 1000*math.pi)
        command("feature.delete", feature_id=source)
        assert "Schizzo sorgente mancante" in tool("feature_get", {"feature_id": ring["feature_id"]})["error"]
        scope["check_volume"](body, 22500 - 160*math.pi)
        tool("undo")
        document = tool("document_inspect")["document"]
        saved = work / "cli-input.sagomacad"
        saved.write_text(json.dumps(document), encoding="utf-8")
        def call(command_id, params):
            reply = subprocess.run([cli, "call", "engine.execute", json.dumps({"command": command_id, "params": params}),
                                    "--headless", str(saved)], capture_output=True, text=True, encoding="utf-8")
            assert reply.returncode == 0, reply.stdout + reply.stderr
            return json.loads(reply.stdout)["result"]
        call("feature.edit", {"feature_id": ring["feature_id"], "params": {"angle": "90 deg"}})
        assert abs(call("model.mass_properties", {"body": ring["body_id"]})["volume_mm3"] - 500*math.pi) < 1e-4
        extra = call("feature.revolve", {"sketch": source, "profiles": [profile], "axis": axis, "type": "full"})
        assert abs(call("model.mass_properties", {"body": extra["body_id"]})["volume_mm3"] - 2000*math.pi) < 1e-4

        cli_script = json.loads(re.search(r"```json\n(\[\n.*?\n\])\n```", guide, re.S).group(1))
        script_file = work / "steps.json"
        script_file.write_text(json.dumps(cli_script), encoding="utf-8")
        reply = subprocess.run([cli, "run", "--script-file", str(script_file), "--headless", str(work / "piastra.sagomacad")],
                               capture_output=True, text=True, encoding="utf-8")
        assert reply.returncode == 0, reply.stdout + reply.stderr
        results = json.loads(reply.stdout)["results"]
        assert abs(results[9]["result"]["volume_mm3"] - (3200-25*math.pi)*15) < 1e-4
        assert not results[11]["result"]["applied"] and not results[11]["result"]["error"]
        assert results[14]["result"]["suppressed"]
        exec(compile(blocks[-1], "modeling-guide-close", "exec"), scope)
    finally:
        process = scope.get("process")
        if process and process.poll() is None:
            process.terminate()
            process.wait(timeout=5)
print("M4 MCP/CLI: executable guide, parametric plate/cut/ring, errors, rollback and L bracket 21997.345175 mm3 OK")
