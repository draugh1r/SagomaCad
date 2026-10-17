"""Generate real boards through SDL/OpenGL; check options, PNG size and themes."""
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import json
import base64

cli = str(Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory() as temporary:
    outputs = []
    for theme in ('light', 'dark'):
        path = Path(temporary) / f'{theme}.png'
        subprocess.run([cli, 'design-board', '--theme', theme, '--out', str(path)], check=True, capture_output=True)
        data = path.read_bytes()
        assert data[:8] == b'\x89PNG\r\n\x1a\n'
        assert struct.unpack('>II', data[16:24]) == (1600, 1800)
        assert len(data) > 30000
        outputs.append(data)
    assert outputs[0] != outputs[1], 'Themes must produce different boards'
    for options in ([], ['--theme', 'unknown'], ['--size', '640x400'], ['--scale', '0'], ['--script', '[["ui.set",{}]]']):
        path = Path(temporary) / 'invalid.png'
        args = options if not options else options + ['--out', str(path)]
        result = subprocess.run([cli, 'design-board', *args], capture_output=True)
        assert result.returncode == 2, (args, result.stdout, result.stderr)
        assert not path.exists()
print('Design boards: two themes, dimensions and invalid options verified')
process = subprocess.Popen([cli, 'mcp', '--headless'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, encoding='utf-8')
def rpc(method, params=None):
    process.stdin.write(json.dumps(dict(jsonrpc='2.0', id=1, method=method, params=params or {}))+'\n')
    process.stdin.flush()
    return json.loads(process.stdout.readline())['result']
try:
    rpc('initialize', {'protocolVersion': '2025-06-18', 'capabilities': {}, 'clientInfo': {'name': 'design-board-test', 'version': '1'}})
    assert 'design_board' in {t['name'] for t in rpc('tools/list')['tools']}
    before = rpc('tools/call', dict(name='document_inspect', arguments={}))['structuredContent']
    for theme in ('light', 'dark'):
        result = rpc('tools/call', dict(name='design_board', arguments=dict(theme=theme)))
        assert not result.get('isError'), result
        assert result['structuredContent']['theme'] == theme
        data = base64.b64decode(result['content'][0]['data'])
        assert data[:8] == b'\x89PNG\r\n\x1a\n'
        assert struct.unpack('>II', data[16:24]) == (1600, 1800)
    after = rpc('tools/call', dict(name='document_inspect', arguments={}))['structuredContent']
    assert before == after, 'Board rendering must preserve the model and history'
    for arguments in ({'theme': 'bad'}, {'theme': False}, {'unknown': True}):
        assert rpc('tools/call', dict(name='design_board', arguments=arguments))['isError']
finally:
    process.stdin.close()
    assert process.wait(timeout=10) == 0, process.stderr.read()
print('MCP design_board: images, validation and document invariance verified')
