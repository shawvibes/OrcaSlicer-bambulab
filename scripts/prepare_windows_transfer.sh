#!/usr/bin/env bash
# Prepare OrcaSlicer-bambulab for transfer to Windows (excludes build artifacts).
set -euo pipefail

REPO_ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
OUTPUT="${1:-$REPO_ROOT/../OrcaSlicer-bambulab-windows-transfer.tar.gz}"

cd "$REPO_ROOT"

echo "Creating transfer archive (source + bridge runtime, no deps/build)..."
echo "Output: $OUTPUT"

tar -czf "$OUTPUT" \
  --exclude='.git' \
  --exclude='build' \
  --exclude='build-*' \
  --exclude='deps/build' \
  --exclude='node-cache' \
  --exclude='**/.build-linux-host' \
  --exclude='**/build-standalone' \
  --exclude='**/__pycache__' \
  --exclude='**/.DS_Store' \
  -C "$(dirname "$REPO_ROOT")" "$(basename "$REPO_ROOT")"

SIZE=$(du -h "$OUTPUT" | cut -f1)
echo ""
echo "Done. Archive size: $SIZE"
echo ""
echo "On Windows:"
echo "  1. Copy $OUTPUT to C:\\src\\"
echo "  2. Extract with 7-Zip or: tar -xzf OrcaSlicer-bambulab-windows-transfer.tar.gz"
echo "  3. Open C:\\src\\OrcaSlicer-bambulab in Cursor"
echo "  4. Follow docs\\WINDOWS_CURSOR_QUICKSTART.md"
