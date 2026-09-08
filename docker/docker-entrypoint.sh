#!/bin/bash
set -e

git submodule update --init --recursive

if git describe --tags --exact-match >/dev/null 2>&1; then
  export SEMVER="$(git describe --tags --exact-match)"
fi

export GITHUB_SHA_SHORT="$(git rev-parse --short HEAD)"

### --- Download HL2SDK-CS2 + Metamod-Source -------------------------------
SDK_DIR="/tmp/sdk"
HL2SDK_DIR="$SDK_DIR/hl2sdk-cs2"
MMSOURCE_DIR="$SDK_DIR/metamod-source"
CSGO_PROTO_DIR="$SDK_DIR/Protobufs"

echo "=== Preparing temporary SDK directory ==="
rm -rf "$SDK_DIR"
mkdir -p "$SDK_DIR"

echo "=== Downloading HL2SDK-CS2 ==="
git clone --recursive --branch cs2 --single-branch https://github.com/alliedmodders/hl2sdk.git "$HL2SDK_DIR"

echo "=== Downloading Metamod-Source ==="
git clone --recursive --branch master --single-branch https://github.com/alliedmodders/metamod-source.git "$MMSOURCE_DIR"

echo "=== Downloading Protobufs ==="
git clone --recursive https://github.com/SteamDatabase/Protobufs "$CSGO_PROTO_DIR"

### --- Export env vars for CMake ------------------------------------------
export HL2SDKCS2="$HL2SDK_DIR"
export MMSOURCE_DEV="$MMSOURCE_DIR"
export CSGO_PROTO="$CSGO_PROTO_DIR/csgo"

echo "Using HL2SDKCS2=$HL2SDKCS2"
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
  --hl2sdk-root=$HL2SDKCS2 \
  --hl2sdk-manifests=$MMSOURCE_DEV/hl2sdk-manifests
ambuild

echo "=== DONE ==="
