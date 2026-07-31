#!/bin/sh
# FexDXVK Non-Adreno wrapper entry point
# Called by Winlator GameHub before Wine/FEX launch

WRAPPER_DIR="$(dirname "$(readlink -f "$0")")"

# Wrapper .so (LD_PRELOAD)
export LD_PRELOAD="$WRAPPER_DIR/prebuilt/wrapper/libfexdxvk_wrapper.so${LD_PRELOAD:+:$LD_PRELOAD}"

# Vulkan implicit layer
export VK_IMPLICIT_LAYER_PATH="$WRAPPER_DIR/layer"
if [ "${FEXDXVK_ENABLE_LAYER:-0}" = "1" ]; then
    export VK_INSTANCE_LAYERS="${VK_INSTANCE_LAYERS:-VK_LAYER_fexdxvk}"
fi

# DXVK + VKD3D DLL overrides
export WINEDLLOVERRIDES="d3d9=n,b;d3d10core=n,b;d3d11=n,b;d3d12=n,b;dxgi=n,b"

# Config paths
export DXVK_CONFIG_FILE="$WRAPPER_DIR/config/dxvk.conf"
export VKD3D_CONFIG_FILE="$WRAPPER_DIR/config/vkd3d_proton.conf"
export FEXDXVK_CONFIG="$WRAPPER_DIR/config/wrapper.json"

# FEX thunks
export FEX_THUNKLIBS="$WRAPPER_DIR/prebuilt/fex/thunks"
export FEX_ROOTFS="${FEX_ROOTFS:-$WRAPPER_DIR/prebuilt/fex/rootfs}"
export FEX_CONFIG="${FEX_CONFIG:-$WRAPPER_DIR/config/fex.conf}"
export FEX_CONFIG_FILE="${FEX_CONFIG_FILE:-$WRAPPER_DIR/config/fex.conf}"

# Simple emulator command. Usage:
#   ./emulator.sh /path/to/game.exe [args...]
export FEXDXVK_ROOT="$WRAPPER_DIR"

# Runtime stats socket
export FEXDXVK_STATS_PATH="/tmp/fexdxvk_stats"

# Low-overhead defaults. Enable diagnostics explicitly when profiling:
#   FEXDXVK_MONITOR=1 FEXDXVK_WORKERS=1 ./init.sh ...
export FEXDXVK_MONITOR="${FEXDXVK_MONITOR:-0}"
export FEXDXVK_WORKERS="${FEXDXVK_WORKERS:-0}"
export FEXDXVK_FRAME_PACING="${FEXDXVK_FRAME_PACING:-0}"

exec "$@"
