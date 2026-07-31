#!/system/bin/sh
# F1 2014 cool high-quality launcher for Winlator GameHub.

PROFILE_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"

export LD_PRELOAD="$PROFILE_DIR/prebuilt/wrapper/libfexdxvk_wrapper.so${LD_PRELOAD:+:$LD_PRELOAD}"
export VK_IMPLICIT_LAYER_PATH="$PROFILE_DIR/layer"
if [ "${FEXDXVK_ENABLE_LAYER:-0}" = "1" ]; then
    export VK_INSTANCE_LAYERS="${VK_INSTANCE_LAYERS:-VK_LAYER_fexdxvk}"
fi

# F1 2014 uses DirectX 10/11; keep native DXVK DLL preference for both
# architectures. The D3D9 and D3D12 overrides remain available for
# compatibility with alternate Wine/GameHub layouts.
export WINEDLLOVERRIDES="d3d8=n,b;d3d9=n,b;d3d10core=n,b;d3d11=n,b;d3d12=n,b;dxgi=n,b"
export DXVK_CONFIG_FILE="$PROFILE_DIR/config/f1-2014-dxvk.conf"
export VKD3D_CONFIG_FILE="$PROFILE_DIR/config/f1-2014-vkd3d.conf"
export FEX_CONFIG_FILE="$PROFILE_DIR/config/f1-2014-fex.conf"
export FEX_CONFIG="$PROFILE_DIR/config/f1-2014-fex.conf"
export FEXDXVK_CONFIG="$PROFILE_DIR/config/f1-2014-wrapper.json"
export FEX_THUNKLIBS="$PROFILE_DIR/prebuilt/fex/thunks"
export FEXDXVK_STATS_PATH="/tmp/f1-2014-fexdxvk-stats"

# Cool, low-overhead defaults.
export FEXDXVK_MONITOR="${FEXDXVK_MONITOR:-0}"
export FEXDXVK_WORKERS="${FEXDXVK_WORKERS:-0}"
export FEXDXVK_FRAME_PACING="${FEXDXVK_FRAME_PACING:-0}"
export FEXDXVK_GPU_PROFILE="${FEXDXVK_GPU_PROFILE:-balanced}"
export FEXDXVK_ROOT="$PROFILE_DIR"

exec "$@"