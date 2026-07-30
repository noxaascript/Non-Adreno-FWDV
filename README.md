# FexDXVK Non-Adreno

Separate DXVK, VKD3D-Proton, FEX, and custom wrapper packages for
**Winlator GameHub** on Android devices using **Mali** or **PowerVR** GPUs.

> This project is not intended for Qualcomm Adreno GPUs. Adreno devices should
> use a regular Turnip/DXVK wrapper instead.

## What is included

This repository contains four separately packaged components:

| Package | Contents | Status |
|---|---|---|
| `FexDXVK-DXVK-3.0.2.winlator` | DXVK 3.0.2 x32/x64 DLLs and tuned `dxvk.conf` | Ready |
| `FexDXVK-VKD3D-Proton-3.0.1.winlator` | VKD3D-Proton 3.0.1 x86/x64 D3D12 DLLs | Ready |
| `FexDXVK-Wrapper-Improved-arm64.winlator` | Custom Android ARM64 wrapper `.so`, Vulkan layer, config, source | Ready |
| `FexDXVK-FEX-2607-source.winlator` | FEX configuration and source/build files | Build required |

The combined `FexDXVK-NonAdreno.winlator` archive is also retained for users
who want the DXVK, VKD3D, and custom wrapper files together.

## Important FEX limitation

FEX-Emu does not publish a compatible prebuilt FEX-2607 Android/ARM64 runtime
asset. The FEX package therefore does **not** contain fake or incompatible
host binaries.

The following files must be built on an ARM64 Linux/Android environment before
FEX emulation is complete:

```text
prebuilt/fex/FEXInterpreter
prebuilt/fex/FEXBash
prebuilt/fex/lib/libFEX.so
prebuilt/fex/lib/libFEXCore.so
prebuilt/fex/thunks/*.so
```

The DXVK, VKD3D-Proton, custom wrapper, and Vulkan layer packages can still be
used independently.

## Requirements

For importing packages:

1. Winlator GameHub with the **Wrappers → Import** feature.
2. An Android device with a Mali or PowerVR Vulkan driver.
3. A 64-bit ARM device for the custom wrapper package.

For building the custom libraries:

- Android NDK r25c or newer.
- CMake 3.18 or newer.
- Android API 28 or newer.
- Vulkan headers, including `vulkan/vk_layer.h`.

## Download packages

The latest binaries are attached to the GitHub release:

<https://github.com/noxaascript/Non-Adreno-FWDV/releases>

Download the components you need:

- `FexDXVK-DXVK-3.0.2.winlator`
- `FexDXVK-VKD3D-Proton-3.0.1.winlator`
- `FexDXVK-Wrapper-Improved-arm64.winlator`
- `FexDXVK-FEX-2607-source.winlator`

Verify downloads with the checksum file:

```bash
sha256sum -c attached_assets/FexDXVK-separate-packages.sha256
```

## Import into Winlator GameHub

Import each archive separately:

1. Copy the `.winlator` file to the Android device.
2. Open **Winlator GameHub**.
3. Open **Wrappers**.
4. Tap **Import**.
5. Select the package.
6. Repeat for each component you want to install.
7. Open the target container.
8. Select **Edit Container → Wrapper**.
9. Choose the imported wrapper.
10. Launch the game and test with the `balanced` profile first.

If GameHub only allows one wrapper to be selected, import the custom wrapper
package first and copy the DXVK/VKD3D DLLs into the wrapper's corresponding
`x32` and `x64` directories using the file manager available in your setup.
The combined archive is easier when the importer supports all files in one
package.

## Build the custom Android libraries

The repository already includes ARM64 binaries in
`prebuilt/wrapper/` and `prebuilt/vulkan_layer/`. Rebuild them when changing
the wrapper source.

### 1. Set up the Android NDK

```bash
export NDK="$HOME/Android/Sdk/ndk/25.2.9519653"
test -f "$NDK/build/cmake/android.toolchain.cmake"
```

### 2. Provide the Vulkan layer header

Some NDK releases include `vulkan.h` but not `vk_layer.h`. Install the
official Khronos header if necessary:

```bash
mkdir -p "$NDK/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include/vulkan"
curl -L \
  https://raw.githubusercontent.com/KhronosGroup/Vulkan-Headers/main/include/vulkan/vk_layer.h \
  -o "$NDK/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include/vulkan/vk_layer.h"
```

### 3. Configure and build

```bash
cmake \
  -S src \
  -B build/android \
  -DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-28 \
  -DCMAKE_BUILD_TYPE=Release \
  -DANDROID_STL=c++_shared

cmake --build build/android --parallel
```

The build creates:

```text
prebuilt/wrapper/libfexdxvk_wrapper.so
prebuilt/vulkan_layer/libVkLayer_fexdxvk.so
```

Both libraries should report `ARM aarch64` when checked with:

```bash
file prebuilt/wrapper/libfexdxvk_wrapper.so
file prebuilt/vulkan_layer/libVkLayer_fexdxvk.so
```

## Build FEX-Emu on ARM64

FEX itself must be built on a compatible ARM64 Linux/Android environment.
Building FEX on an x86 workstation produces host binaries that must not be
placed in this package.

```bash
git clone --branch FEX-2607 --depth 1 https://github.com/FEX-Emu/FEX.git
cmake \
  -S FEX \
  -B FEX/build \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_THUNKS=ON \
  -DCMAKE_INSTALL_PREFIX="$PWD/fex-install"

cmake --build FEX/build --parallel
cmake --install FEX/build
```

Copy the resulting ARM64 files into the package:

```bash
cp fex-install/bin/FEX prebuilt/fex/FEXInterpreter
cp fex-install/bin/FEXBash prebuilt/fex/FEXBash
cp fex-install/lib/libFEX*.so prebuilt/fex/lib/
cp fex-install/lib/fex-emu/thunks/*.so prebuilt/fex/thunks/
```

Confirm the architecture before packaging:

```bash
file prebuilt/fex/FEXInterpreter
file prebuilt/fex/FEXBash
file prebuilt/fex/lib/*.so
```

## Obtain DXVK and VKD3D-Proton

The repository release already contains the official runtime DLLs. To replace
them with a newer release, unpack the upstream archives and preserve this
layout:

```text
prebuilt/dxvk/x32/d3d8.dll
prebuilt/dxvk/x32/d3d9.dll
prebuilt/dxvk/x32/d3d10core.dll
prebuilt/dxvk/x32/d3d11.dll
prebuilt/dxvk/x32/dxgi.dll
prebuilt/dxvk/x64/...

prebuilt/vkd3d/x32/d3d12.dll
prebuilt/vkd3d/x32/d3d12core.dll
prebuilt/vkd3d/x64/d3d12.dll
prebuilt/vkd3d/x64/d3d12core.dll
```

Do not mix x32 and x64 DLLs. DXVK x32 files are Windows 32-bit PE DLLs;
the wrapper and Vulkan layer are Android ARM64 ELF libraries.

## Package the components locally

From the repository root:

```bash
rm -rf /tmp/fexdxvk-packages
mkdir -p /tmp/fexdxvk-packages

zip -qr /tmp/fexdxvk-packages/FexDXVK-DXVK-3.0.2.winlator \
  config/dxvk.conf prebuilt/dxvk README.md

zip -qr /tmp/fexdxvk-packages/FexDXVK-VKD3D-Proton-3.0.1.winlator \
  config/vkd3d_proton.conf prebuilt/vkd3d README.md

zip -qr /tmp/fexdxvk-packages/FexDXVK-Wrapper-Improved-arm64.winlator \
  config/wrapper.json config/fex.conf config/dxvk.conf \
  layer/ prebuilt/wrapper/ prebuilt/vulkan_layer/ src/ init.sh README.md

zip -qr /tmp/fexdxvk-packages/FexDXVK-NonAdreno.winlator \
  config/ layer/ prebuilt/ src/ init.sh README.md
```

Test every archive:

```bash
unzip -t /tmp/fexdxvk-packages/*.winlator
```

Generate checksums:

```bash
sha256sum /tmp/fexdxvk-packages/*.winlator
```

## Configuration

### Wrapper profile

Edit `config/wrapper.json`:

| Profile | Thermal threshold | Intended use |
|---|---:|---|
| `balanced` | 75°C | Recommended starting point |
| `performance` | 85°C | Higher sustained performance |
| `cool` | 65°C | Lower temperature and power |

### Low-overhead mode

The default configuration is now tuned to reduce avoidable frame-time jitter:

- Worker threads are disabled unless explicitly requested.
- DXVK HUD/file polling is disabled unless diagnostics are enabled.
- The custom frame-pacing sleep is disabled by default so it does not
  double-throttle the Android compositor.
- DXVK uses two shader compiler threads, a persistent state cache, and one
  frame of maximum latency.

Enable diagnostics only while profiling:

```bash
FEXDXVK_MONITOR=1 FEXDXVK_WORKERS=1 ./init.sh <wine-or-fex-command>
```

Enable the custom pacing layer only if the device exhibits visible cadence
judder with the compositor's normal pacing:

```bash
FEXDXVK_FRAME_PACING=1 ./init.sh <wine-or-fex-command>
```

These changes target CPU wakeups, duplicate present sleeps, and shader
compilation stalls. A specific 34% stutter reduction must be measured on the
target phone and game; this project does not claim a guaranteed percentage
without that device benchmark.

### DXVK settings

`config/dxvk.conf` contains conservative mobile-GPU defaults:

- Persistent state cache.
- Moderate command-buffer chunk size.
- Two frames in flight.
- Frame pacing through a one-frame present interval.
- Conservative raw SSBO behavior for non-Adreno drivers.
- Pipeline library support where the driver exposes it.

If a game has rendering problems, temporarily test with:

```text
dxvk.logLevel = debug
```

Return it to `warn` after troubleshooting.

### Runtime entry point

`init.sh` sets:

- `LD_PRELOAD` for the custom wrapper.
- Vulkan implicit-layer discovery.
- Wine DLL overrides for DXVK/VKD3D.
- Configuration paths.
- FEX thunk and statistics paths.

Run it only with the intended Wine/FEX command:

```bash
./init.sh <wine-or-fex-command> [arguments...]
```

## Runtime diagnostics

The wrapper writes:

```text
/tmp/fexdxvk_stats
/tmp/fexdxvk_perf_scale
```

Useful environment variables:

```bash
FEXDXVK_CONFIG=/path/to/config/wrapper.json
FEXDXVK_LAYER_ENABLE=1
FEXDXVK_STATS_PATH=/tmp/fexdxvk_stats
DXVK_HUD=fps,frametime
```

Check the startup log for:

```text
[fexdxvk] GPU detected
[fexdxvk] ready
[fexdxvk-thermal] sensor
[fexdxvk-mon] monitor started
```

## Troubleshooting

### Wrapper does not appear in GameHub

- Confirm the file ends in `.winlator`.
- Import it through **Wrappers → Import**, not as a game archive.
- Re-download and verify the SHA-256 checksum.
- Try the standalone wrapper archive instead of the combined archive.

### Game starts but has no graphics

- Confirm the device is Mali or PowerVR.
- Start with `balanced`.
- Confirm `VK_INSTANCE_LAYERS` and `VK_IMPLICIT_LAYER_PATH` are set by
  `init.sh`.
- Temporarily disable the custom layer by removing
  `VK_INSTANCE_LAYERS` to determine whether the issue is layer-related.

### 32-bit games fail

- Confirm all files under `prebuilt/dxvk/x32/` are 32-bit DLLs.
- Confirm `d3d12.dll` and `d3d12core.dll` are present in the VKD3D x32 folder.
- Do not use x64 DLLs in a 32-bit Wine prefix.

### Device becomes too hot

- Change the profile to `cool`.
- Lower the thermal threshold in `config/wrapper.json`.
- Stop using `performance` for long sessions.
- Check `/tmp/fexdxvk_stats` for temperature and performance scale.

### FEX reports an executable or architecture error

- Do not use FEX binaries built on x86.
- Rebuild FEX on ARM64.
- Verify every FEX executable and `.so` with `file`.
- Confirm thunk libraries are under `prebuilt/fex/thunks/`.

## Project layout

```text
config/                 Runtime configuration
layer/                  Vulkan implicit-layer manifest
prebuilt/dxvk/          DXVK Windows PE DLLs
prebuilt/vkd3d/         VKD3D-Proton Windows PE DLLs
prebuilt/fex/           FEX runtime slots; build required
prebuilt/wrapper/       Custom Android ARM64 wrapper library
prebuilt/vulkan_layer/  Custom Android ARM64 Vulkan layer
src/wrapper/            Wrapper source
src/vulkan_layer/       Vulkan layer source
src/CMakeLists.txt      Android/Linux build
init.sh                 Runtime environment entry point
```

## License and upstream projects

DXVK, VKD3D-Proton, FEX-Emu, and Vulkan headers retain their respective
upstream licenses. Review each upstream project before redistribution.

- DXVK: <https://github.com/doitsujin/dxvk>
- VKD3D-Proton: <https://github.com/HansKristian-Work/vkd3d-proton>
- FEX-Emu: <https://github.com/FEX-Emu/FEX>
- Vulkan-Headers: <https://github.com/KhronosGroup/Vulkan-Headers>