#!/bin/bash
# Assemble the GitHub Pages site (browser installer + face simulator) from
# built firmware artifacts.
#
# Usage: scripts/assemble_pages.sh <artifacts-dir>
#   <artifacts-dir> must contain release/ and debug/ subdirectories with the
#   .bin files (bootloader, partitions, boot_app0, firmware, littlefs).
#
# The www/ directory is the Pages site root: the installer page, manifests,
# firmware artifacts, update.json (for the clock's self-update check), and
# the face simulator at simulator/.
set -euo pipefail

ARTS="$1"

mkdir -p www/artifacts www/debug/artifacts www/simulator
cp "$ARTS"/release/*.bin www/artifacts/
cp "$ARTS"/debug/*.bin www/debug/artifacts/
cp www/debug/manifest-debug.json www/debug/manifest.json
cp simulator/index.html www/simulator/index.html

VERSION=$(tr -d '[:space:]' < data/version.txt)
FW_SHA=$(sha256sum www/artifacts/firmware.bin | cut -d' ' -f1)
FS_SHA=$(sha256sum www/artifacts/littlefs.bin | cut -d' ' -f1)
cat > www/update.json <<EOF
{
  "version": "$VERSION",
  "firmware": { "url": "artifacts/firmware.bin", "sha256": "$FW_SHA" },
  "filesystem": { "url": "artifacts/littlefs.bin", "sha256": "$FS_SHA" }
}
EOF

echo "Site assembled in www/ (version $VERSION)"
