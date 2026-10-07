#! /bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Build the browser simulator in the official Emscripten Docker image, at the version pinned in
# scripts/emsdk_version.txt. Then serve it on http://localhost:8000 with --serve.

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
REPO_DIR="$(dirname -- "${SCRIPT_DIR}")"
EMSDK_VERSION_FILE="${SCRIPT_DIR}/emsdk_version.txt"
EMSDK_IMAGE_NAME="emscripten/emsdk"
CONTAINER_REPO_DIR="/project"
SERVE_PORT=8000

usage() {
  cat <<USAGE
Usage: scripts/simulator_docker.sh [--serve]

Build tools/simulator in the ${EMSDK_IMAGE_NAME} Docker image.

Options:
  --serve      After the build, serve the simulator on http://localhost:${SERVE_PORT}.
               Stop the server with Ctrl+C.
  -h, --help   Show this help.
USAGE
}

require_command() {
  local command_name="$1"
  local reason="$2"

  if ! command -v "${command_name}" &>/dev/null; then
    printf 'error: "%s" is not installed. It is needed to %s.\n' "${command_name}" "${reason}" >&2
    return 1
  fi
}

main() {
  local serve=0
  local emsdk_version

  while [[ $# -gt 0 ]]; do
    case "$1" in
      --serve)
        serve=1
        shift
        ;;
      -h | --help)
        usage
        return 0
        ;;
      *)
        printf 'error: unknown argument %s\n' "$1" >&2
        usage >&2
        return 1
        ;;
    esac
  done

  require_command docker "run the Emscripten container"
  emsdk_version="$(tr -d '[:space:]' <"${EMSDK_VERSION_FILE}")"

  docker run \
    --rm \
    --user "$(id -u):$(id -g)" \
    --env HOME=/tmp \
    --volume "${REPO_DIR}:${CONTAINER_REPO_DIR}" \
    --workdir "${CONTAINER_REPO_DIR}" \
    "${EMSDK_IMAGE_NAME}:${emsdk_version}" \
    scripts/ci.sh simulator

  if [[ "${serve}" -eq 1 ]]; then
    require_command python3 "serve the simulator page"
    printf 'Open http://localhost:%d in a browser.\n' "${SERVE_PORT}"
    python3 -m http.server "${SERVE_PORT}" --directory "${REPO_DIR}/tools/simulator/web"
  fi
}

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  set -euo pipefail
  main "$@"
fi
