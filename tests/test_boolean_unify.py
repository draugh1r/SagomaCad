"""Unified boolean topology and downstream naming through MCP and CLI."""
import json
import math
from pathlib import Path
import subprocess
import sys
from test_mcp_integration import Client

cli = sys.argv[1]
fixture = json.loads((Path(__file__).parent / "scenarios/boolean-l.json").read_text(encoding="utf-8"))
client = Client([cli, "mcp", "--headless"])
try:
    def command(name, **params):
        return client.ok("command_run", {"command": name, "params": params})
    for index, (_, step) in enumerate(fixture):
        command(step["command"], **step["params"])
        if index == 1:
            assert len(command("document.inspect")["document"]["bodies"]) == 1
            assert len(client.ok("geometry_query", {"body":"body-1", "kind":"face"})["entities"]) == 6
            assert abs(client.ok("mass_properties", {"body":"body-1"})["volume_mm3"] - 8000) < 1e-5
    assert len(command("document.inspect")["document"]["bodies"]) == 1
    assert len(client.ok("geometry_query", {"body":"body-1", "kind":"face"})["entities"]) == 8
    assert abs(client.ok("mass_properties", {"body":"body-1"})["volume_mm3"] - 12000) < 1e-5
    edges = client.ok("geometry_query", {"body":"body-1", "kind":"edge", "parallel_to":[0,1,0]})["entities"]
    chosen = [e for e in edges if abs(e["geometry"]["center"][0] - 40) < 1e-7 and abs(e["geometry"]["center"][2] - 10) < 1e-7]
    assert len(chosen) == 1, chosen
    params = {"edges":[chosen[0]["ref"]], "radius":"2 mm"}
    rounded = command("feature.fillet", **params)
    command("feature.edit", feature_id="feature-2", params={"distance":"30 mm"})
    assert client.ok("feature_get", {"feature_id":rounded["feature_id"]})["error"] == ""
    expected = 14000 - 20*(4-math.pi)
    assert abs(client.ok("mass_properties", {"body":"body-1"})["volume_mm3"] - expected) < 1e-5
finally:
    client.close()

def execute(name, params):
    return ["engine.execute", {"command":name, "params":params}]
script = fixture + [execute("geometry.query", {"body":"body-1", "kind":"face"}),
    execute("document.inspect", {}), execute("model.mass_properties", {"body":"body-1"}),
    execute("feature.fillet", params),
    execute("feature.edit", {"feature_id":"feature-2", "params":{"distance":"30 mm"}}),
    execute("feature.get", {"feature_id":rounded["feature_id"]}),
    execute("model.mass_properties", {"body":"body-1"})]
reply = subprocess.run([cli, "run", "--script", json.dumps(script)], capture_output=True, text=True, encoding="utf-8", timeout=60)
assert reply.returncode == 0, reply.stderr + reply.stdout
results = json.loads(reply.stdout)["results"]
assert len(results[len(fixture)]["result"]["entities"]) == 8
assert len(results[len(fixture)+1]["result"]["document"]["bodies"]) == 1
assert abs(results[len(fixture)+2]["result"]["volume_mm3"] - 12000) < 1e-5
assert results[-2]["result"]["error"] == ""
assert abs(results[-1]["result"]["volume_mm3"] - expected) < 1e-5
print("MCP and CLI: unified 6/8 faces, one body, exact volume and stable moving fillet OK")
