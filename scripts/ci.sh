#! /bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# The CI commands. GitHub Actions runs them in the espressif/idf image. Run them
# locally with the same image:
#   docker run --rm -v "$PWD:/project" -w /project espressif/idf:v6.1 scripts/ci.sh all

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
REPO_DIR="$(dirname -- "${SCRIPT_DIR}")"
HOST_TEST_PACKAGES=(build-essential libbsd-dev)

usage() {
  cat <<USAGE
Usage: scripts/ci.sh COMMAND

Commands:
  host-tests                Build and run the host tests.
  build APP [VARIANT]       Build firmware/APP. VARIANT selects firmware/APP/sdkconfig.ci.VARIANT.
  simulator                 Build the browser simulator in tools/simulator/web. Needs Emscripten
                            (emcmake) on the PATH, for example in the emscripten/emsdk image.
  release VERSION           Build the game and the bring-up firmware and write one merged image
                            of each (flash offset 0x0) to dist/.
  all                       Run the host tests and build every app and variant.
USAGE
}

load_idf() {
  if command -v idf.py &>/dev/null; then
    return 0
  fi

  if [[ -z "${IDF_PATH:-}" ]]; then
    printf 'error: IDF_PATH is not set. Run this script in the espressif/idf image or load ESP-IDF first.\n' >&2
    return 1
  fi

  # shellcheck source=/dev/null
  source "${IDF_PATH}/export.sh" >/dev/null
}

install_host_test_packages() {
  local missing=()
  local package

  for package in "${HOST_TEST_PACKAGES[@]}"; do
    if ! dpkg -s "${package}" &>/dev/null; then
      missing+=("${package}")
    fi
  done

  if [[ "${#missing[@]}" -eq 0 ]]; then
    return 0
  fi

  apt-get update
  apt-get install -y "${missing[@]}"
}

run_host_tests() {
  install_host_test_packages
  load_idf
  "${SCRIPT_DIR}/run_host_tests.sh"
}

build_app() {
  local app="$1"
  local variant="${2:-}"
  local app_dir="${REPO_DIR}/firmware/${app}"
  # A separate build directory, so CI settings never stay in the cache of a normal
  # idf.py build in build/.
  local build_dir="${app_dir}/build_ci_default"
  local defaults="sdkconfig.defaults"

  if [[ ! -d "${app_dir}" ]]; then
    printf 'error: firmware/%s does not exist.\n' "${app}" >&2
    return 1
  fi

  if [[ -n "${variant}" ]]; then
    if [[ ! -f "${app_dir}/sdkconfig.ci.${variant}" ]]; then
      printf 'error: firmware/%s/sdkconfig.ci.%s does not exist.\n' "${app}" "${variant}" >&2
      return 1
    fi
    defaults="sdkconfig.defaults;sdkconfig.ci.${variant}"
    build_dir="${app_dir}/build_ci_${variant}"
  fi

  load_idf
  printf '\n=== Building firmware/%s %s ===\n' "${app}" "${variant:-(default)}"
  idf.py \
    -C "${app_dir}" \
    -B "${build_dir}" \
    -D SDKCONFIG="${build_dir}/sdkconfig" \
    -D SDKCONFIG_DEFAULTS="${defaults}" \
    build
}

build_simulator() {
  local source_dir="${REPO_DIR}/tools/simulator"
  local build_dir="${source_dir}/build"

  if ! command -v emcmake &>/dev/null; then
    printf 'error: emcmake not found. Load emsdk, or use scripts/simulator_docker.sh.\n' >&2
    return 1
  fi

  emcmake cmake -S "${source_dir}" -B "${build_dir}" -DCMAKE_BUILD_TYPE=Release
  cmake --build "${build_dir}" --parallel
  printf 'Simulator built: %s/web/index.html\n' "${source_dir}"
}

merge_app() {
  local app="$1"
  local variant="$2"
  local output="$3"
  local build_dir="${REPO_DIR}/firmware/${app}/build_ci_${variant:-default}"

  idf.py -C "${REPO_DIR}/firmware/${app}" -B "${build_dir}" -D SDKCONFIG="${build_dir}/sdkconfig" \
    merge-bin -o "${output}"
}

build_release() {
  local version="$1"
  local dist_dir="${REPO_DIR}/dist"

  mkdir -p "${dist_dir}"
  build_app life_torus
  build_app life_torus bringup
  merge_app life_torus "" "${dist_dir}/life_torus-${version}.bin"
  merge_app life_torus bringup "${dist_dir}/life_torus-bringup-${version}.bin"
  (cd "${dist_dir}" && sha256sum ./*.bin >"SHA256SUMS-${version}.txt")
  printf 'Release images in %s\n' "${dist_dir}"
}

run_all() {
  run_host_tests
  build_app life_torus
  build_app life_torus bringup
}

main() {
  local command="${1:-}"

  case "${command}" in
    host-tests)
      run_host_tests
      ;;
    build)
      if [[ $# -lt 2 ]]; then
        usage >&2
        return 1
      fi
      build_app "$2" "${3:-}"
      ;;
    release)
      if [[ $# -lt 2 ]]; then
        usage >&2
        return 1
      fi
      build_release "$2"
      ;;
    simulator)
      build_simulator
      ;;
    all)
      run_all
      ;;
    -h | --help)
      usage
      ;;
    *)
      usage >&2
      return 1
      ;;
  esac
}

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  set -euo pipefail
  main "$@"
fi
