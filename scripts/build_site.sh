#! /bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Assemble the GitHub Pages site in site/: the browser simulator as the main page, and the web
# flasher with the merged firmware images. Build the simulator (scripts/ci.sh simulator) and the
# release images (scripts/ci.sh release VERSION) first.

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
REPO_DIR="$(dirname -- "${SCRIPT_DIR}")"
SIMULATOR_DIR="${REPO_DIR}/tools/simulator/web"
SITE_SOURCE_DIR="${REPO_DIR}/tools/site"
DIST_DIR="${REPO_DIR}/dist"
SITE_DIR="${REPO_DIR}/site"

usage() {
  cat <<USAGE
Usage: scripts/build_site.sh VERSION

Copies the simulator from tools/simulator/web, the flasher page from tools/site and the
separate images in dist/parts/game and dist/parts/bringup into site/.
USAGE
}

# The manifest flashes the three separate images, not the merged image. The merged image covers
# the NVS partition, so it would erase the saved settings.
write_manifest() {
  local path="$1"
  local name="$2"
  local version="$3"
  local folder="$4"

  cat >"${path}" <<MANIFEST
{
  "name": "${name}",
  "version": "${version}",
  "new_install_prompt_erase": true,
  "builds": [
    {
      "chipFamily": "ESP32-S3",
      "parts": [
        { "path": "${folder}/bootloader.bin", "offset": 0 },
        { "path": "${folder}/partition-table.bin", "offset": 32768 },
        { "path": "${folder}/life_torus.bin", "offset": 65536 }
      ]
    }
  ]
}
MANIFEST
}

# Adds ?v=VERSION to the script and stylesheet addresses, so a browser loads the new files after
# each deploy and not old files from its cache.
add_version() {
  local page="$1"
  local version="$2"

  sed -i -E "s/(src|href)=\"([^\":]+\.(js|css))\"/\1=\"\2?v=${version}\"/g" "${page}"
}

main() {
  local version="${1:-}"

  if [[ -z "${version}" ]]; then
    usage >&2
    return 1
  fi
  if [[ ! -f "${SIMULATOR_DIR}/life_torus.js" ]]; then
    printf 'error: the simulator is not built. Run scripts/ci.sh simulator first.\n' >&2
    return 1
  fi
  if [[ ! -f "${DIST_DIR}/parts/game/life_torus.bin" ]]; then
    printf 'error: dist/parts/game not found. Run scripts/ci.sh release %s first.\n' \
      "${version}" >&2
    return 1
  fi

  rm -rf "${SITE_DIR}"
  mkdir -p "${SITE_DIR}/firmware"
  cp "${SIMULATOR_DIR}/index.html" "${SIMULATOR_DIR}/app.js" "${SIMULATOR_DIR}/style.css" \
    "${SIMULATOR_DIR}/life_torus.js" "${SITE_DIR}/"
  cp -r "${REPO_DIR}/tools/phone_app" "${SITE_DIR}/phone"
  sed "s/VERSION/${version}/" "${SITE_SOURCE_DIR}/flash.html" >"${SITE_DIR}/flash.html"
  cp -r "${DIST_DIR}/parts/game" "${DIST_DIR}/parts/bringup" "${SITE_DIR}/firmware/"
  write_manifest "${SITE_DIR}/firmware/manifest.json" "Life Torus" "${version}" "game"
  write_manifest "${SITE_DIR}/firmware/manifest-bringup.json" "Life Torus bring-up" "${version}" \
    "bringup"
  add_version "${SITE_DIR}/index.html" "${version}"
  add_version "${SITE_DIR}/phone/index.html" "${version}"
  sed -i "s#\.\./life_torus\.js\"#../life_torus.js?v=${version}\"#" "${SITE_DIR}/phone/app.js"
  printf 'Site in %s\n' "${SITE_DIR}"
}

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  set -euo pipefail
  main "$@"
fi
