#!/system/bin/sh
# FexDXVK simple emulator launcher.
# Usage: ./emulator.sh /path/to/program.exe [arguments...]

set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
FEX="${FEX_INTERPRETER:-$ROOT/prebuilt/fex/FEXInterpreter}"

if [ "$#" -lt 1 ]; then
    echo "Usage: $0 /path/to/program.exe [arguments...]" >&2
    exit 64
fi

if [ ! -x "$FEX" ]; then
    echo "FEX runtime is missing: $FEX" >&2
    echo "Build FEX-2607 for ARM64 Android/Linux and install FEXInterpreter," >&2
    echo "FEXBash, libFEX.so, libFEXCore.so, thunks, and RootFS under prebuilt/fex." >&2
    exit 127
fi

for required in \
    "$ROOT/prebuilt/wrapper/libfexdxvk_wrapper.so" \
    "$ROOT/prebuilt/vulkan_layer/libVkLayer_fexdxvk.so" \
    "$ROOT/layer/VkLayer_fexdxvk.json"; do
    if [ ! -f "$required" ]; then
        echo "Package is incomplete; missing: $required" >&2
        exit 66
    fi
done

# Use the normal wrapper setup, then invoke FEX with the Windows program.
# FEX itself handles x86/x86_64 selection from the executable. The optional
# Vulkan layer can be enabled with FEXDXVK_ENABLE_LAYER=1.
. "$ROOT/init.sh"
exec "$FEX" "$@"