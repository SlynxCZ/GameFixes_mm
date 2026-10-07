#!/bin/bash
set -e

git submodule update --init --recursive

# SEMVER comes in from the CI, whose version job resolves the tag -- on the
# pushbuild.txt path it is not on HEAD yet, so git describe cannot see it. A
# tag push and a local build still find it on HEAD.
if [ -n "${SEMVER:-}" ]; then
  export SEMVER
  echo "=== Version: $SEMVER (from the environment) ==="
elif SEMVER="$(git describe --tags --exact-match 2>/dev/null)"; then
  export SEMVER
  echo "=== Version: $SEMVER (tag on HEAD) ==="
else
  unset SEMVER
fi

export GITHUB_SHA_SHORT="$(git rev-parse --short HEAD)"

### --- Download s2sdk + Metamod-Source ------------------------------------
SDK_DIR="/tmp/sdk"
S2SDK_DIR="$SDK_DIR/s2sdk"
MMSOURCE_DIR="$SDK_DIR/metamod-source"
CSGO_PROTO_DIR="$SDK_DIR/Protobufs"

echo "=== Preparing temporary SDK directory ==="
rm -rf "$SDK_DIR"
mkdir -p "$SDK_DIR"

echo "=== Downloading s2sdk ==="
git clone --recursive --branch cs2 --single-branch https://github.com/alliedmodders/s2sdk.git "$S2SDK_DIR"

echo "=== Downloading Metamod-Source ==="
git clone --recursive --branch master --single-branch https://github.com/alliedmodders/metamod-source.git "$MMSOURCE_DIR"

echo "=== Downloading Protobufs ==="
git clone --recursive https://github.com/SteamTracking/Protobufs "$CSGO_PROTO_DIR"

### --- Export env vars for CMake ------------------------------------------
export S2SDK="$S2SDK_DIR"
export MMSOURCE_DEV="$MMSOURCE_DIR"
export CSGO_PROTO="$CSGO_PROTO_DIR/csgo"

echo "Using S2SDK=$S2SDK"
echo "Using MMSOURCE_DEV=$MMSOURCE_DEV"
echo "Using CSGO_PROTO=$CSGO_PROTO"

### --- Build ---------------------------------------------------------------
echo "=== Starting build ==="

rm -rf build
mkdir build
cd build

pwd
python ../configure.py \
  --enable-optimize \
  --sdks cs2 \
  --mms_path=$MMSOURCE_DEV \
  --hl2sdk-manifests=$MMSOURCE_DEV/hl2sdk-manifests
ambuild

echo "=== DONE ==="
