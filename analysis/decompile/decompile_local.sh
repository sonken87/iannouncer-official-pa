#!/usr/bin/env bash
# Local (Linux) attempt to decompile engine.jsc with jsc2js's prebuilt d8.
#
# NOTE: The Linux d8 aborts on this win32-produced code cache (vector OOB).
# This script documents that and is kept for reproducibility; the working
# path is the Windows runner in .github/workflows/decompile-engine-jsc.yml.
set -u
JSC="${1:-$(dirname "$0")/../../engine.jsc}"
TAG="${2:-15.2.124.28}"
WORK="$(mktemp -d)"
echo "work dir: $WORK"; cd "$WORK"

echo "[1/4] download patched d8 (linux) for V8 $TAG"
curl -fSL -o d8.tgz \
  "https://github.com/xqy2006/jsc2js/releases/download/$TAG/d8-$TAG-linux.tar.gz"
tar xzf d8.tgz && chmod +x d8

echo "[2/4] provide icudtl.dat"
ICU="$(find / -name icudtl.dat 2>/dev/null | head -1)"
[ -n "$ICU" ] && cp "$ICU" ./icudtl.dat || echo "WARNING: no icudtl.dat found"

echo "[3/4] disassemble via loadjsc"
./d8 -e "loadjsc('$(readlink -f "$JSC")')" > engine.bytecode.txt 2> d8.err.txt
echo "d8 exit=$?  bytecode lines=$(wc -l < engine.bytecode.txt)"
head -20 d8.err.txt

echo "[4/4] (if bytecode produced) run View8"
if [ -s engine.bytecode.txt ]; then
  git clone --depth 1 https://github.com/xqy2006/jsc2js jsc2js
  python3 jsc2js/View8/view8.py --disassembled engine.bytecode.txt engine.js
  echo "engine.js size: $(stat -c%s engine.js 2>/dev/null)"
else
  echo "No bytecode produced (Linux d8 rejected the cache). Use the Windows workflow."
fi
echo "outputs in: $WORK"
