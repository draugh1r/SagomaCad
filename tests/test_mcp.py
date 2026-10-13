import json
import subprocess
import sys


def main():
    process = subprocess.Popen(
        [sys.argv[1], "mcp", "--headless"],
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        text=True, encoding="utf-8", bufsize=1,
    )
    assert process.stdin and process.stdout

    def call(method, params=None, request_id=1):
        request = {"jsonrpc": "2.0", "id": request_id, "method": method}
        if params is not None:
            request["params"] = params
        process.stdin.write(json.dumps(request) + "\n")
        process.stdin.flush()
        result = json.loads(process.stdout.readline())
        assert result["jsonrpc"] == "2.0" and result["id"] == request_id, result
        return result

    try:
        initialized = call("initialize", {"protocolVersion": "2025-06-18", "capabilities": {}, "clientInfo": {"name": "test", "version": "1"}})
        assert initialized["result"]["serverInfo"]["name"] == "sagomacad"
        process.stdin.write('{"jsonrpc":"2.0","method":"notifications/initialized"}\n')
        process.stdin.flush()
        tools = call("tools/list")["result"]["tools"]
        names = {tool["name"] for tool in tools}
        assert {"commands_list", "command_run", "batch_run", "undo", "redo", "document_inspect", "render_view", "mass_properties", "export", "ui_screenshot"} <= names
        commands = call("tools/call", {"name": "commands_list", "arguments": {}})["result"]["structuredContent"]
        assert any(command["id"] == "param.set" for command in commands)
        set_result = call("tools/call", {"name": "command_run", "arguments": {"command": "param.set", "params": {"name": "larghezza", "expression": "40 mm"}}})
        assert not set_result["result"].get("isError"), set_result
        document = call("tools/call", {"name": "document_inspect", "arguments": {}})["result"]["structuredContent"]
        assert document["parameters"][0]["value"] == 40
        resources = call("resources/list")["result"]["resources"]
        assert {r["uri"] for r in resources} == {"sagomacad://document", "sagomacad://commands", "sagomacad://docs/modeling-guide"}
        for resource in resources:
            answer = call("resources/read", {"uri": resource["uri"]})["result"]["contents"][0]
            assert answer["uri"] == resource["uri"] and answer["text"]
        assert call("does/not/exist")["error"]["code"] == -32601
        assert call("tools/call", {"name": 3})["error"]["code"] == -32602
        assert call("resources/read", {"uri": "sagomacad://missing"})["error"]["data"]["code"] == "mcp.unknown_resource"
        process.stdin.write("{bad json\n")
        process.stdin.flush()
        assert json.loads(process.stdout.readline())["error"]["code"] == -32700
        failure = call("tools/call", {"name": "command_run", "arguments": {"command": "unknown.command"}})["result"]
        assert failure["isError"] and failure["structuredContent"]["code"] == "command.failed"
    finally:
        process.stdin.close()
        assert process.wait(timeout=5) == 0, process.stderr.read()


if __name__ == "__main__":
    main()
