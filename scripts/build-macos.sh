#!/bin/zsh
set -euo pipefail

root="${0:A:h:h}"
arch="$(uname -m)"
build_dir="${root}/build/macos-${arch}"
dist_dir="${root}/dist"

if ! command -v brew >/dev/null; then
    print -u2 "Homebrew is required to locate libuv, hwloc, and OpenSSL."
    exit 1
fi

cmake -S "${root}" -B "${build_dir}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES="${arch}" \
    -DOPENSSL_ROOT_DIR="$(brew --prefix openssl@3)"
cmake --build "${build_dir}" --parallel

mkdir -p "${dist_dir}"
rm -rf "${dist_dir}/rxs-${arch}.app"
cp -R "${build_dir}/rxs.app" "${dist_dir}/rxs-${arch}.app"
codesign --force --deep --sign - "${dist_dir}/rxs-${arch}.app"
ditto -c -k --sequesterRsrc --keepParent "${dist_dir}/rxs-${arch}.app" "${dist_dir}/rxs-${arch}.zip"

print "Built ${dist_dir}/rxs-${arch}.app and ${dist_dir}/rxs-${arch}.zip"
