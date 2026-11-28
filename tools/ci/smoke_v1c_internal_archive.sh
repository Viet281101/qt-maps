#!/usr/bin/env bash
set -euo pipefail

# The archive directory is mounted read-only at /input. Docker must use
# --network none; the real artifact never leaves the local machine.
[[ -z "$(ip -4 route show default)" && -z "$(ip -6 route show default)" ]]
cd /input
sha256sum -c SHA256SUMS
mkdir -p /proof/archive
tar --zstd --no-same-owner -xf qtmaps-v1-candidate-internal-linux-x86_64.tar.zst \
    -C /proof/archive
python3 - <<'PY'
import hashlib
import json
from pathlib import Path

outer = json.loads(Path('/input/manifest.json').read_text())
root = Path('/proof/archive')
inner = json.loads((root / 'manifest.json').read_text())
assert inner == outer
assert inner['kind'] == 'internal-real-data-review-candidate'
assert inner['target'] == 'ubuntu-26.04-x86_64'
assert len(inner['source_commit']) == 40
for name, expected in inner['review_files_sha256'].items():
    assert hashlib.sha256((root / name).read_bytes()).hexdigest() == expected, name
assert hashlib.sha256((root / 'stage/metadata/SHA256SUMS').read_bytes()).hexdigest() == inner['stage_sha256s_sha256']
assert hashlib.sha256((root / 'stage/app/qtmaps').read_bytes()).hexdigest() == inner['app_sha256']
assert (root / 'LICENSES/NOTICE').is_file()
assert (root / 'operator/README.md').is_file()
print('Candidate archive manifest, evidence, notices and app hash: passed')
PY
bash /smoke-stage.sh /proof/archive/stage /proof/archive/install.sh
