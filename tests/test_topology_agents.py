"""Face support is reachable through MCP command_run and the headless CLI."""
import json
import math
import subprocess
import sys
from test_mcp_integration import Client

cli = sys.argv[1]
steps = [
    ("primitive.add", {"kind": "box", "dimensions": {"width": "40 mm", "depth": "40 mm", "height": "20 mm"}}),
    ("sketch.create", {"face": {"body": "body-1", "name": "primitive-1/z_max"}}),
    ("sketch.circle", {"x": 10, "y": 10, "diameter": "10 mm"}),
    ("feature.extrude", {"sketch": "sketch-2", "profiles": [["e1"]], "distance": "6 mm", "operation": "new"}),
    ("feature.edit", {"feature_id": "primitive-1", "dimensions": {"height": "35 mm"}}),
    ("sketch.inspect", {"sketch": "sketch-2"}),
    ("model.mass_properties", {"body": "body-3"}),
    ("feature.extrude", {"face": {"body": "body-1", "name": "primitive-1/z_max"}, "distance": "4 mm", "operation": "new"}),
    ("feature.edit", {"feature_id": "primitive-1", "dimensions": {"height": "40 mm"}}),
    ("model.mass_properties", {"body": "body-4"}),
]
client = Client([cli, "mcp", "--headless"])
try:
    for command, params in steps:
        result = client.ok("command_run", {"command": command, "params": params})
        if command == "sketch.inspect":
            assert len(result["entities"]) == 5 and result["origin"][2] == 35, result
            assert all(e["construction"] for e in result["entities"] if "reference_name" in e)
    assert abs(result["bbox_mm"]["min"]["z"] - 40) < 1e-5, result
finally:
    client.close()
script = [["engine.execute", {"command": c, "params": p}] for c, p in steps]
completed = subprocess.run([cli, "run", "--script", json.dumps(script)], capture_output=True, text=True, encoding="utf-8", timeout=60)
assert completed.returncode == 0, completed.stderr + completed.stdout
reply = json.loads(completed.stdout)
assert abs(reply["results"][-1]["result"]["bbox_mm"]["min"]["z"] - 40) < 1e-5, reply
print("Face support, projection, upstream change and cylinder position through MCP and CLI OK")

edge_refs = [{"body": "body-1", "name": f"primitive-1/edge(primitive-1/x_{x},primitive-1/y_{y})"}
             for x in ("min", "max") for y in ("min", "max")]
for command, amount in (("feature.fillet", "radius"), ("feature.chamfer", "distance")):
    dress_steps = [("primitive.add", {"kind": "box", "dimensions": {"width": "40 mm", "depth": "40 mm", "height": "20 mm"}}),
                   (command, {"edges": edge_refs, amount: "2 mm"}),
                   ("model.mass_properties", {"body": "body-1"})]
    expected = (1600 - (4-math.pi if amount == "radius" else 2)*4)*20
    client = Client([cli, "mcp", "--headless"])
    try:
        for command_id, params in dress_steps:
            result = client.ok("command_run", {"command": command_id, "params": params})
        assert abs(result["volume_mm3"] - expected) < 1e-4, result
    finally:
        client.close()
    script = [["engine.execute", {"command": c, "params": p}] for c, p in dress_steps]
    completed = subprocess.run([cli, "run", "--script", json.dumps(script)], capture_output=True, text=True, encoding="utf-8", timeout=60)
    assert completed.returncode == 0, completed.stderr + completed.stdout
    assert abs(json.loads(completed.stdout)["results"][-1]["result"]["volume_mm3"] - expected) < 1e-4
print("Multiple stable edges: fillet and chamfer through MCP and CLI OK")

hole_steps = [
    ("primitive.add", {"kind": "box", "dimensions": {"width": "40 mm", "depth": "40 mm", "height": "20 mm"}}),
    ("feature.hole", {"face": {"body": "body-1", "name": "primitive-1/z_max"}, "position": {"x": 10, "y": 10}, "diameter": "8 mm", "through_all": True}),
    ("feature.edit", {"feature_id": "primitive-1", "dimensions": {"width": "60 mm", "height": "35 mm"}}),
    ("model.mass_properties", {"body": "body-1"}),
]
expected = 84000 - 16*math.pi*35
client = Client([cli, "mcp", "--headless"])
try:
    for command_id, params in hole_steps:
        result = client.ok("command_run", {"command": command_id, "params": params})
    assert abs(result["volume_mm3"] - expected) < 1e-4, result
    params = client.ok("feature_get", {"feature_id": "feature-2"})["params"]
    assert len(params["references"]) == 2
    assert all(r["distance"] == "10 mm" and r["edge"]["signature"]["kind"] == "edge" for r in params["references"])
finally:
    client.close()
script = [["engine.execute", {"command": c, "params": p}] for c, p in hole_steps]
completed = subprocess.run([cli, "run", "--script", json.dumps(script)], capture_output=True, text=True, encoding="utf-8", timeout=60)
assert completed.returncode == 0, completed.stderr + completed.stdout
assert abs(json.loads(completed.stdout)["results"][-1]["result"]["volume_mm3"] - expected) < 1e-4
print("Hole with two stable position dimensions through MCP and CLI OK")

client = Client([cli, "mcp", "--headless"])
try:
    client.ok("command_run", {"command": "primitive.add", "params": {"kind": "box", "dimensions": {"width": "40 mm", "depth": "40 mm", "height": "20 mm"}}})
    vertical = client.ok("geometry_query", {"kind": "edge", "type": "line", "parallel_to": [0, 0, 1]})["entities"]
    assert len(vertical) == 4 and all(e["name"] and e["description"] for e in vertical)
    top = client.ok("geometry_query", {"kind": "face", "normal": [0, 0, 1]})["entities"][0]["ref"]
    bottom = client.ok("geometry_query", {"kind": "face", "normal": [0, 0, -1]})["entities"][0]["ref"]
    measurement = client.ok("measure", {"a": top, "b": bottom})
    assert abs(measurement["distance_mm"] - 20) < 1e-6 and measurement["angle_degrees"] == 0, measurement
    assert client.call("selection_get")["isError"]
    client.ok("command_run", {"command": "feature.fillet", "params": {"edges": [e["ref"] for e in vertical], "radius": "2 mm"}})
    assert len(client.ok("geometry_query", {"kind": "face"})["entities"]) == 10
finally:
    client.close()
script = [["engine.execute", {"command": c, "params": p}] for c, p in [
    ("primitive.add", {"kind": "box", "dimensions": {"width": "40 mm", "depth": "40 mm", "height": "20 mm"}}),
    ("geometry.query", {"kind": "edge", "parallel_to": [0, 0, 1]}),
    ("inspect.measure", {"a": {"body": "body-1", "name": "primitive-1/z_max", "kind": "face"}, "b": {"body": "body-1", "name": "primitive-1/z_min", "kind": "face"}}),
]]
completed = subprocess.run([cli, "run", "--script", json.dumps(script)], capture_output=True, text=True, encoding="utf-8", timeout=60)
assert completed.returncode == 0, completed.stderr + completed.stdout
reply = json.loads(completed.stdout)["results"]
assert len(reply[1]["result"]["entities"]) == 4 and abs(reply[2]["result"]["distance_mm"] - 20) < 1e-6
print("Geometry query, measure, headless selection refusal and all-vertical-edge agent workflow OK")

# Automatic extrusion is shared by headless MCP and CLI command dispatch.
auto_steps = [
    ("primitive.add", {"kind": "box", "dimensions": {"width": "10 mm", "depth": "20 mm", "height": "30 mm"}}),
    ("feature.extrude.suggest", {"face": {"body": "body-1", "name": "primitive-1/x_max"}, "distance": "7 mm"}),
    ("feature.extrude", {"face": {"body": "body-1", "name": "primitive-1/x_max"}, "distance": "7 mm"}),
    ("model.mass_properties", {"body": "body-1"}),
]
client = Client([cli, "mcp", "--headless"])
try:
    for command, params in auto_steps:
        result = client.ok("command_run", {"command": command, "params": params})
        if command.endswith("suggest"):
            assert result == {"operation": "join", "target_bodies": ["body-1"]}, result
        if command == "feature.extrude":
            assert result["body_id"] == "body-1", result
    assert abs(result["volume_mm3"] - 10200) < 1e-5, result
    assert abs(result["area_mm2"] - 2900) < 1e-5, result
finally:
    client.close()
reply = subprocess.run([cli, "run", "--script", json.dumps([["engine.execute", {"command": c, "params": p}] for c, p in auto_steps])], capture_output=True, text=True, encoding="utf-8", timeout=60)
assert reply.returncode == 0, reply.stderr + reply.stdout
assert abs(json.loads(reply.stdout)["results"][-1]["result"]["volume_mm3"] - 10200) < 1e-5
