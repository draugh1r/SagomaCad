"""Exercise CLI session discovery, control calls, headless run and PNG capture."""
import json
import socket
import subprocess
import sys
import tempfile
import time
from pathlib import Path

app = Path(sys.argv[1])
cli = Path(sys.argv[2])

def invoke(*args, success=True):
    completed = subprocess.run([str(cli), *map(str, args)], capture_output=True,
                               text=True, encoding="utf-8", timeout=15)
    if success and completed.returncode:
        raise AssertionError(completed.stderr)
    if not success:
        assert completed.returncode != 0 and completed.stderr
        return None
    return json.loads(completed.stdout)

with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    token = root / "token.txt"
    token.write_text("cli-integration-secret", encoding="utf-8")
    session = root / "session.json"
    with socket.socket() as probe:
        probe.bind(("127.0.0.1", 0))
        port = probe.getsockname()[1]
    process = subprocess.Popen([str(app), "--control", str(port),
                                "--control-token-file", str(token),
                                "--session-file", str(session),
                                "--automation-root", str(root)],
                               stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    try:
        deadline = time.monotonic() + 15
        while not session.exists():
            if process.poll() is not None:
                raise AssertionError(process.stderr.read().decode(errors="replace"))
            if time.monotonic() > deadline:
                raise TimeoutError("session file not written")
            time.sleep(.1)
        options = ("--session-file", session)
        commands = invoke("call", "engine.commands", *options)["result"]
        assert any(row["id"] == "param.set" for row in commands)

        def execute(command, params=None):
            return invoke("call", "engine.execute",
                          json.dumps({"command": command, "params": params or {}}), *options)["result"]
        execute("param.set", {"name": "larghezza", "expression": "40 mm"})
        execute("param.set", {"name": "foro", "expression": "larghezza / 4"})
        assert execute("param.list")[1]["value"] == 10
        execute("param.set", {"name": "larghezza", "expression": "80 mm"})
        assert execute("param.list")[1]["value"] == 20
        execute("edit.undo")
        assert execute("param.list")[1]["value"] == 10
        execute("edit.redo")
        assert execute("param.list")[1]["value"] == 20
        saved = root / "control.sagomacad"
        execute("document.saveAs", {"path": str(saved)})
        assert saved.is_file()
        shot = Path(invoke("call", "ui.screenshot", json.dumps({"path": "cli.png"}),
                           *options)["result"]["path"])
        assert shot.read_bytes()[:8] == b"\x89PNG\r\n\x1a\n"
        invoke("call", "engine.execute", json.dumps({"command": "missing"}),
               *options, success=False)
    finally:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait(timeout=5)

    headless = root / "headless.sagomacad"
    script = [["engine.execute", {"command": "param.set", "params":
                                  {"name": "larghezza", "expression": "40 mm"}}],
              ["engine.execute", {"command": "param.set", "params":
                                  {"name": "foro", "expression": "larghezza / 4"}}],
              ["engine.execute", {"command": "param.set", "params":
                                  {"name": "larghezza", "expression": "80 mm"}}],
              ["engine.execute", {"command": "edit.undo"}],
              ["engine.execute", {"command": "param.list"}],
              ["engine.execute", {"command": "edit.redo"}],
              ["engine.execute", {"command": "param.list"}]]
    result = invoke("run", "--script", json.dumps(script), "--headless", headless)
    assert result["results"][4]["result"][1]["value"] == 10
    assert result["results"][6]["result"][1]["value"] == 20
    assert headless.exists()
    listed = invoke("call", "engine.execute", json.dumps({"command": "param.list"}),
                    "--headless", headless)["result"]
    assert listed[1]["value"] == 20

print("CLI control, session discovery, headless run and screenshot passed")
