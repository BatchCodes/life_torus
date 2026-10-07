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
images dist/life_torus-VERSION.bin and dist/life_torus-bringup-VERSION.bin into site/.
USAGE
}

write_manifest() {
  local path="$1"
  local name="$2"
  local version="$3"
  local image="$4"

  cat >"${path}" <<MANIFEST
{
  "name": "${name}",
  "version": "${version}",
  "new_install_prompt_erase": true,
  "builds": [
    {
      "chipFamily": "ESP32-S3",
      "parts": [{ "path": "${image}", "offset": 0 }]
    }
  ]
}
MANIFEST
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
  if [[ ! -f "${DIST_DIR}/life_torus-${version}.bin" ]]; then
    printf 'error: dist/life_torus-%s.bin not found. Run scripts/ci.sh release %s first.\n' \
      "${version}" "${version}" >&2
    return 1
  fi

  rm -rf "${SITE_DIR}"
  mkdir -p "${SITE_DIR}/firmware"
  cp "${SIMULATOR_DIR}/index.html" "${SIMULATOR_DIR}/app.js" "${SIMULATOR_DIR}/style.css" \
    "${SIMULATOR_DIR}/life_torus.js" "${SITE_DIR}/"
  sed "s/VERSION/${version}/" "${SITE_SOURCE_DIR}/flash.html" >"${SITE_DIR}/flash.html"
  cp "${DIST_DIR}/life_torus-${version}.bin" "${SITE_DIR}/firmware/life_torus.bin"
  cp "${DIST_DIR}/life_torus-bringup-${version}.bin" "${SITE_DIR}/firmware/life_torus-bringup.bin"
  write_manifest "${SITE_DIR}/firmware/manifest.json" "Life Torus" "${version}" "life_torus.bin"
  write_manifest "${SITE_DIR}/firmware/manifest-bringup.json" "Life Torus bring-up" "${version}" \
    "life_torus-bringup.bin"
  printf 'Site in %s\n' "${SITE_DIR}"
}

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  set -euo pipefail
  main "$@"
fi
