"""Insert the user-requested corrective phase without renumbering existing phases."""
import json
from datetime import datetime, timezone
from pathlib import Path

path = Path('docs/.plan/tasks.json')
data = json.loads(path.read_text(encoding='utf-8'))
titles = [
    ('Riferimenti 40–42 e criteri di regressione', 'VERIFY'),
    ('Preferenza di visibilità dello schizzo e riuso dei profili', 'MODIFY'),
    ('Operazione automatica di estrusione e scelta manuale persistente', 'MODIFY'),
    ('Quote sui tratti durante disegno e modifica', 'MODIFY'),
    ('Verifica finale M5: scenari, GProp e confronto visivo', 'VERIFY'),
]
if not any(t['phase'] == 9.5 for t in data['tasks']):
    last = max(t['id'] for t in data['tasks'])
    for i, (title, action) in enumerate(titles, 1):
        data['tasks'].append(dict(id=last+i, phase=9.5, title=f'9.5.{i-1} {title}',
            status='pending', priority='high', dependencies=[19] if i == 1 else [last+i-1],
            files=['docs/.plan/prps/phase-9.5-correzioni-m5.md'], action=action,
            validation='Build Release, ctest e controlli locali come da PRP',
            features=[title], isTest=action == 'VERIFY', milestone='M5'))
    for t in data['tasks']:
        if t['phase'] == 10 and last+5 not in t['dependencies']:
            t['dependencies'].append(last+5)
    data['meta']['currentPhase'] = 9.5
    data['meta']['updatedAt'] = datetime.now(timezone.utc).isoformat()
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
