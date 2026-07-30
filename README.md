# FexDXVK Non-Adreno

Winlator GameHub wrapper packages for Android devices with **Mali** or
**PowerVR** GPUs. The project provides separate DXVK, VKD3D-Proton, FEX
configuration, and custom ARM64 wrapper components.

> Not intended for Qualcomm Adreno GPUs. Use a Turnip-based wrapper on Adreno.

## Latest release

The current release is **v1.1.0**:

<https://github.com/noxaascript/Non-Adreno-FWDV/releases/tag/v1.1.0>

### Recommended package for F1 2014

**FexDXVK-F1-2014-Cool.winlator**

This is a game-specific profile for the 32-bit Direct3D 9 version of F1 2014.
It keeps high visual quality while reducing avoidable heat and frame-time
spikes:

- DXVK 3.0.2 x32 runtime for D3D9.
- Stable 60 FPS-oriented presentation.
- Persistent shader/state cache.
- Two DXVK compiler threads instead of an unrestricted compiler storm.
- No idle worker-pool polling.
- No duplicate custom frame-pacing sleep by default.
- Thermal threshold of 70°C.
- Diagnostics and HUD polling disabled during normal play.

## Downloadable packages

| Package | Approx. size | Use |
|---|---:|---|
| `FexDXVK-F1-2014-Cool.winlator` | 23 MB | Recommended F1 2014 profile |
| `FexDXVK-DXVK-3.0.2.winlator` | 18 MB | Standalone DXVK |
| `FexDXVK-VKD3D-Proton-3.0.1.winlator` | 5 MB | Standalone D3D12 runtime |
| `FexDXVK-Wrapper-Improved-arm64.winlator` | 125 KB | Custom wrapper and Vulkan layer |
| `FexDXVK-FEX-2607-source.winlator` | 30 KB | FEX configuration and source |
| `FexDXVK-NonAdreno.winlator` | 23 MB | Combined general-purpose package |

Checksums are included in the release. Always verify a download before
copying it to Android.

## Import into Winlator GameHub

1. Download a `.winlator` file from the release.
2. Copy it to the Android device.
3. Open **Winlator GameHub**.
4. Open **Wrappers → Import**.
5. Select the package.
6. Open the F1 2014 container.
7. Choose **Edit Container → Wrapper**.
8. Select the imported FexDXVK wrapper.
9. Launch the game.

For F1 2014, use a **32-bit Wine prefix** and start with the balanced-cool
profile. If GameHub requires one combined archive, use
`FexDXVK-F1-2014-Cool.winlator`.

## F1 2014 package layout

```text
FexDXVK-F1-2014-Cool.winlator
├── config/
│   ├── f1-2014-dxvk.conf
│   ├── f1-2014-fex.conf
│   ├── f1-2014-vkd3d.conf
│   └── f1-2014-wrapper.json
├── init-f1-2014.sh
├── layer/VkLayer_fexdxvk.json
└── prebuilt/
    ├── dxvk/x32/       F1 2014's 32-bit DXVK DLLs
    ├── dxvk/x64/       64-bit DXVK DLLs
    ├── vkd3d/x32/      32-bit D3D12 DLLs
    ├── vkd3d/x64/      64-bit D3D12 DLLs
    ├── wrapper/        Android ARM64 wrapper .so
    ├── vulkan_layer/   Android ARM64 Vulkan layer .so
    └── fex/            FEX runtime slot
```

F1 2014 normally uses `prebuilt/dxvk/x32/d3d9.dll` and `dxgi.dll`.
The VKD3D files are included because some GameHub wrapper importers expect
both graphics API layouts.

## FEX runtime requirement

The repository includes FEX configuration, but does not include fake or
incompatible FEX binaries. FEX-Emu does not publish a compatible prebuilt
FEX-2607 Android/ARM64 runtime package.

Build FEX on an ARM64 Android/Linux environment and add:

```text
prebuilt/fex/FEXInterpreter
prebuilt/fex/FEXBash
prebuilt/fex/lib/libFEX.so
prebuilt/fex/lib/libFEXCore.so
prebuilt/fex/thunks/*.so
```

Do not copy x86 workstation FEX binaries into an Android package.

## Build FEX-Emu on ARM64

```bash
git clone --depth 1 --branch FEX-2607 \
  https://github.com/FEX-Emu/FEX.git

cmake -S FEX -B FEX/build \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_THUNKS=ON \
  -DCMAKE_INSTALL_PREFIX="$PWD/fex-install"

cmake --build FEX/build --parallel
cmake --install FEX/build

cp fex-install/bin/FEX prebuilt/fex/FEXInterpreter
cp fex-install/bin/FEXBash prebuilt/fex/FEXBash
cp fex-install/lib/libFEX*.so prebuilt/fex/lib/
cp fex-install/lib/fex-emu/thunks/*.so prebuilt/fex/thunks/
```

Verify architecture:

```bash
file prebuilt/fex/FEXInterpreter
file prebuilt/fex/FEXBash
file prebuilt/fex/lib/*.so
```

Every FEX executable and library must be ARM64/aarch64.

## Build the custom ARM64 wrapper

The repository already includes rebuilt ARM64 libraries. Rebuild them after
changing `src/`:

```bash
export NDK="$HOME/Android/Sdk/ndk/25.2.9519653"

cmake -S src -B build/android \
  -DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-28 \
  -DCMAKE_BUILD_TYPE=Release \
  -DANDROID_STL=c++_shared

cmake --build build/android --parallel
```

Outputs:

```text
prebuilt/wrapper/libfexdxvk_wrapper.so
prebuilt/vulkan_layer/libVkLayer_fexdxvk.so
```

Confirm with:

```bash
file prebuilt/wrapper/libfexdxvk_wrapper.so
file prebuilt/vulkan_layer/libVkLayer_fexdxvk.so
```

## F1 2014 thermal and performance tuning

The F1 profile is intentionally cooler than an uncapped performance profile.

Recommended starting point:

- 60 FPS cap.
- Native 1280x720 or 1920x1080 resolution.
- Keep texture quality high.
- Lower resolution before lowering texture quality if the device overheats.
- Let the first complete lap compile shaders, then restart the game once.
- Use the balanced-cool profile for sustained sessions.

The profile sets a 70°C thermal threshold. To make it cooler, edit
`config/f1-2014-wrapper.json`:

```json
"thermalThresholdCelsius": 68
```

To temporarily collect diagnostics:

```bash
FEXDXVK_MONITOR=1 ./init-f1-2014.sh <wine-or-fex-command>
```

Do not leave monitoring enabled during normal play because HUD and file
polling add overhead.

## Low-overhead defaults

The custom wrapper defaults are designed to avoid wrapper-induced stutter:

- Worker threads are disabled unless `FEXDXVK_WORKERS=1`.
- DXVK HUD/stat polling is disabled unless `FEXDXVK_MONITOR=1`.
- Custom frame-pacing sleeps are disabled unless
  `FEXDXVK_FRAME_PACING=1`.
- DXVK uses a persistent state cache.
- Compiler threads are bounded.
- Thermal scaling remains active to avoid prolonged throttling.

These settings target avoidable frame-time spikes. A guaranteed percentage
improvement requires testing the same game, driver, resolution, and device
before and after the change.

## Repackage locally

From the repository root:

```bash
zip -qr FexDXVK-F1-2014-Cool.winlator \
  profiles/f1-2014/config \
  profiles/f1-2014/init-f1-2014.sh \
  profiles/f1-2014/README.md \
  layer/ prebuilt/

zip -qr FexDXVK-NonAdreno.winlator \
  config/ layer/ prebuilt/ src/ init.sh README.md \
  -x '*/.gitkeep'

unzip -t FexDXVK-F1-2014-Cool.winlator
sha256sum FexDXVK-F1-2014-Cool.winlator
```

## Troubleshooting

### F1 2014 does not start

- Use a 32-bit Wine prefix.
- Confirm `prebuilt/dxvk/x32/d3d9.dll` and `dxgi.dll` exist.
- Confirm the FEX runtime is ARM64, not x86.
- Try removing `VK_INSTANCE_LAYERS` from the launcher to isolate the custom
  Vulkan layer.
- Start at 1280x720 and increase resolution after confirming stability.

### The game is too hot

- Enable a 60 FPS cap.
- Use the `cool` profile or lower the thermal threshold to 68°C.
- Disable custom frame pacing unless the compositor visibly judders.
- Keep diagnostics disabled.

### Stutter on the first lap

This is usually shader compilation. Complete one lap, exit normally, and
launch again so the persistent DXVK state cache can be reused.

### Wrapper does not appear

- Confirm the file extension is `.winlator`.
- Import through **Wrappers → Import**.
- Verify the archive with `unzip -t`.
- Re-download and compare the SHA-256 checksum.

## Upstream projects

- DXVK: <https://github.com/doitsujin/dxvk>
- VKD3D-Proton: <https://github.com/HansKristian-Work/vkd3d-proton>
- FEX-Emu: <https://github.com/FEX-Emu/FEX>
- Vulkan-Headers: <https://github.com/KhronosGroup/Vulkan-Headers>

Each upstream component retains its own license. Review those licenses before
redistributing modified packages.