set -e

REPO_DIR="$(pwd)"
REPO_NAME="$(basename "$REPO_DIR")"
OUTPUT="../${REPO_NAME}-working-tree.zip"

rm -f "$OUTPUT"

zip -r "$OUTPUT" . \
    -x ".git/*" \
    -x "*/.git/*" \
    -x "*/bin/*" \
    -x "*/obj/*" \
    -x "*/node_modules/*" \
    -x "*/dist/*" \
    -x "*/.angular/*" \
    -x "*.zip"

echo "Napravljen ZIP: $OUTPUT"
