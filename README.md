# FexDXVK Non-Adreno

## Intro

FexDXVK is a Winlator GameHub wrapper project for Android devices using
**Mali** or **PowerVR** graphics. It combines DXVK, VKD3D-Proton, FEX
configuration, and a custom ARM64 wrapper/layer focused on stable graphics,
lower heat, and reduced frame-time spikes.

The recommended game profile is:

```text
FexDXVK-F1-2014-Cool.winlator
```

It is tuned for F1 2014's DirectX 10 and DirectX 11 rendering paths. The
package includes both 32-bit and 64-bit DXVK files so the selected Wine
prefix can use the correct architecture.

> This project is not for Qualcomm Adreno GPUs. Use a Turnip-based wrapper
> on Adreno devices.

## Steps

### 1. Download

Open the latest release:

<https://github.com/noxaascript/Non-Adreno-FWDV/releases>

For F1 2014, download:

```text
FexDXVK-F1-2014-Cool.winlator
```

For general use, download:

```text
FexDXVK-NonAdreno.winlator
```

### 2. Import

1. Copy the `.winlator` file to Android.
2. Open **Winlator GameHub**.
3. Open **Wrappers → Import**.
4. Select the package.
5. Edit the game container.
6. Select **Wrapper** and choose the imported wrapper.
7. Launch the game.

### 3. Run a Windows program directly

The package includes a simple emulator launcher:

```bash
./emulator.sh /path/to/game.exe
```

It checks that the ARM64 FEX runtime and wrapper files exist, loads the
DXVK environment, and starts the executable through FEX. The optional custom
Vulkan layer is disabled by default for maximum compatibility; enable it only
for testing with:

```bash
FEXDXVK_ENABLE_LAYER=1 ./emulator.sh /path/to/game.exe
```

It fails clearly when the FEX runtime has not been installed; configuration
files alone are not an emulator.

Use the Wine prefix architecture required by your F1 2014 installation and
start with a 60 FPS cap.

### 4. First launch

The first launch may stutter while shaders compile. Drive one complete lap,
exit normally, and launch again so the persistent shader cache can be reused.

### 5. Tune temperature

Start with the included cool profile. If the phone is still too hot:

- Keep the game capped at 60 FPS.
- Lower resolution before lowering texture quality.
- Change the F1 profile thermal threshold from `70` to `68`.
- Keep diagnostics disabled during normal play.

### 6. Build FEX when required

The package includes FEX configuration, but not fake host binaries. Build
FEX-2607 on an ARM64 Android/Linux environment:

```bash
git clone --depth 1 --branch FEX-2607 \
  https://github.com/FEX-Emu/FEX.git

cmake -S FEX -B FEX/build \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_THUNKS=ON \
  -DCMAKE_INSTALL_PREFIX="$PWD/fex-install"

cmake --build FEX/build --parallel
cmake --install FEX/build
```

Copy the ARM64 results into:

```text
prebuilt/fex/FEXInterpreter
prebuilt/fex/FEXBash
prebuilt/fex/lib/libFEX.so
prebuilt/fex/lib/libFEXCore.so
prebuilt/fex/thunks/*.so
```

Verify them:

```bash
file prebuilt/fex/FEXInterpreter
file prebuilt/fex/FEXBash
file prebuilt/fex/lib/*.so
```

Every file must be ARM64/aarch64.

## Information

### Components

| Component | Contents |
|---|---|
| FEX | FEXCore, FEXInterpreter, FEXBash, RootFS/thunks, and ARM64 libraries |
| Wrapper | Launcher script, environment variables, JSON profiles, ARM64 helper libraries |
| DXVK | `x32/` and `x64/` Direct3D-to-Vulkan DLLs |
| VKD3D-Proton | `x32/` and `x64/` D3D12 translation DLLs |
| Vulkan layer | Pipeline cache, command-buffer reuse, queue handling, and optional pacing |

Typical compressed sizes:

```text
FEX:             20–80 MB
Wrapper:         50 KB–5 MB
DXVK:            8–25 MB
VKD3D-Proton:    3–12 MB
```

The current package is smaller when FEX runtime binaries are not yet added.

### Current packages

| Package | Purpose |
|---|---|
| `FexDXVK-F1-2014-Cool.winlator` | F1 2014 cool high-quality profile |
| `FexDXVK-DXVK-3.0.2.winlator` | Standalone DXVK |
| `FexDXVK-VKD3D-Proton-3.0.1.winlator` | Standalone VKD3D-Proton |
| `FexDXVK-Wrapper-Improved-arm64.winlator` | Custom wrapper and Vulkan layer |
| `FexDXVK-FEX-2607-source.winlator` | FEX configuration and build source |
| `FexDXVK-NonAdreno.winlator` | Combined package |

### Low-overhead defaults

The wrapper is configured to avoid unnecessary frame-time work:

- Worker polling is disabled by default.
- HUD and statistics polling are disabled by default.
- Custom frame-pacing sleeps are disabled by default.
- DXVK shader compilation is bounded.
- State and pipeline caches are persistent.
- Thermal scaling remains active to reduce sustained throttling.

Enable diagnostics only while testing:

```bash
FEXDXVK_MONITOR=1 ./init-f1-2014.sh <wine-or-fex-command>
```

A guaranteed percentage improvement requires testing the same game, device,
driver, resolution, and graphics settings before and after the change.

### Android wrapper build

The custom libraries can be rebuilt with Android NDK r25c or newer:

```bash
export NDK="$HOME/Android/Sdk/ndk/25.2.9519653"

cmake -S src -B build/android \
  -DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-28 \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build/android --parallel
```

Outputs:

```text
prebuilt/wrapper/libfexdxvk_wrapper.so
prebuilt/vulkan_layer/libVkLayer_fexdxvk.so
```

### Troubleshooting

**The wrapper does not appear:** confirm the file ends in `.winlator` and use
**Wrappers → Import**.

**F1 2014 does not start:** confirm the selected Wine prefix architecture and
that the matching DXVK directory contains `d3d10core.dll`, `d3d11.dll`, and
`dxgi.dll`.

**The game is too hot:** cap at 60 FPS, lower resolution, use the cool profile,
and lower the thermal threshold.

**The game stutters on the first lap:** allow shader compilation to finish,
exit normally, then launch again.

**FEX reports an architecture error:** rebuild FEX on ARM64 and verify every
binary with `file`. Do not use x86 host binaries.

## Credits

- **DXVK** — <https://github.com/doitsujin/dxvk>
- **VKD3D-Proton** — <https://github.com/HansKristian-Work/vkd3d-proton>
- **FEX-Emu** — <https://github.com/FEX-Emu/FEX>
- **Vulkan-Headers** — <https://github.com/KhronosGroup/Vulkan-Headers>
- **Winlator GameHub** — the target wrapper/import environment

DXVK, VKD3D-Proton, FEX-Emu, and Vulkan-Headers retain their own upstream
licenses. Review the upstream licenses before redistribution.

## Outro

Start with the F1 2014 cool package, a 60 FPS cap, and the balanced settings.
Let the first lap compile shaders before judging performance. If the device
still overheats, lower resolution or use a cooler thermal threshold rather
than immediately disabling visual quality.

Project releases:

<https://github.com/noxaascript/Non-Adreno-FWDV/releases>