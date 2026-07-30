# FexDXVK NonAdreno Wrapper
**Wrapper-NonAdreno | FexCore-2607-custom | DXVK 3.0.2 official release**

Custom FEX+DXVK+VKD3D-Proton wrapper for Winlator GameHub.
Optimized for **Mali** and **PowerVR** GPUs (not for Adreno / Qualcomm).

---

## Binary Components

| File | Type | Source |
|------|------|--------|
| `prebuilt/dxvk/{x32,x64}/*.dll` | Windows PE DLL (x86/x64) | DXVK 3.0.2 official release build |
| `prebuilt/vkd3d/{x32,x64}/d3d12.dll` | Windows PE DLL (x86/x64) | VKD3D-Proton 3.0.1 build |
| `prebuilt/fex/FEXInterpreter` | ARM64 ELF executable | FEX-Emu 2607 (ARM64 build required) build |
| `prebuilt/fex/FEXBash` | ARM64 ELF executable | FEX-Emu 2607 (ARM64 build required) build |
| `prebuilt/fex/lib/libFEX.so` | ARM64 ELF .so | FEX-Emu 2607 (ARM64 build required) build |
| `prebuilt/wrapper/libfexdxvk_wrapper.so` | ARM64 ELF .so | **Build from src/ (see below)** |
| `prebuilt/vulkan_layer/libVkLayer_fexdxvk.so` | ARM64 ELF .so | **Build from src/ (see below)** |
| `layer/VkLayer_fexdxvk.json` | Vulkan layer manifest (JSON) | Included |

---

## Build the Custom .so Files

The two custom libraries must be compiled for your target ABI (arm64-v8a).

### Android NDK (recommended for Winlator)

```bash
export NDK=$HOME/Android/Sdk/ndk/25.2.9519653

cmake -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake \
      -DANDROID_ABI=arm64-v8a \
      -DANDROID_PLATFORM=android-28 \
      -DCMAKE_BUILD_TYPE=Release \
      -S src -B build/android

cmake --build build/android --parallel
```

Built files will be auto-copied to `prebuilt/wrapper/` and `prebuilt/vulkan_layer/`.

### Linux ARM64 cross-compile

```bash
sudo apt install gcc-aarch64-linux-gnu cmake vulkan-headers

cmake -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
      -DCMAKE_BUILD_TYPE=Release \
      -S src -B build/linux

cmake --build build/linux --parallel
```

---

## Get the Upstream Binaries

### DXVK 3.0.2 official release

```bash
# Build from source (requires MinGW cross-compiler):
git clone https://github.com/<nonadreno-fork>/dxvk.git -b 1.7.5-nonadreno-fix
cd dxvk && ./package-release.sh master /tmp/dxvk-pkg --no-package
cp /tmp/dxvk-pkg/x32/*.dll prebuilt/dxvk/x32/
cp /tmp/dxvk-pkg/x64/*.dll prebuilt/dxvk/x64/
```

### VKD3D-Proton 3.0.1

```bash
git clone https://github.com/HansKristian-Work/vkd3d-proton.git
cd vkd3d-proton && ./package-release.sh master /tmp/vkd3d-pkg
cp /tmp/vkd3d-pkg/x86/d3d12.dll prebuilt/vkd3d/x32/
cp /tmp/vkd3d-pkg/x64/d3d12.dll prebuilt/vkd3d/x64/
```

### FEX-Emu 2607 (ARM64 build required)

```bash
git clone https://github.com/FEX-Emu/FEX.git -b FEX-2607
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_THUNKS=ON \
      -DCMAKE_INSTALL_PREFIX=/tmp/fex-install \
      -DTARGET_ARCH=AARCH64 -S FEX -B FEX/build
cmake --build FEX/build --parallel && cmake --install FEX/build
cp /tmp/fex-install/bin/FEXInterpreter prebuilt/fex/
cp /tmp/fex-install/bin/FEXBash        prebuilt/fex/
cp /tmp/fex-install/lib/libFEX*.so     prebuilt/fex/lib/
cp /tmp/fex-install/lib/fex-emu/thunks/*.so prebuilt/fex/thunks/
```

---

## Import into Winlator GameHub

Once `prebuilt/` is populated with all binaries:

```bash
# Package as .winlator archive
zip -r FexDXVK-NonAdreno.winlator \
    config/ layer/ prebuilt/ src/ init.sh README.md
```

1. Copy `FexDXVK-NonAdreno.winlator` to your Android device.
2. Open **Winlator GameHub** → **Wrappers** → **Import**.
3. Select the `.winlator` file.
4. Assign to a container: **Edit Container** → **Wrapper** → `Wrapper-NonAdreno`.

---

## File Structure

```
FexDXVK-NonAdreno.winlator (ZIP)
├── config/
│   ├── wrapper.json           ← Master config & feature toggles
│   ├── dxvk.conf              ← DXVK Vulkan translation settings
│   ├── fex.conf               ← FEX-Emu JIT/thunk settings
│   └── vkd3d_proton.conf      ← VKD3D-Proton D3D12 settings
├── layer/
│   └── VkLayer_fexdxvk.json   ← Vulkan implicit layer manifest
├── prebuilt/
│   ├── dxvk/{x32,x64}/        ← DXVK .dll (Windows PE)
│   ├── vkd3d/{x32,x64}/       ← VKD3D-Proton .dll (Windows PE)
│   ├── fex/                   ← FEXInterpreter, FEXBash, libFEX.so
│   ├── wrapper/               ← libfexdxvk_wrapper.so (ARM64)
│   └── vulkan_layer/          ← libVkLayer_fexdxvk.so (ARM64)
├── src/
│   ├── CMakeLists.txt
│   ├── vulkan_layer/          ← Vulkan layer C source
│   │   ├── vk_layer_fexdxvk.c
│   │   ├── pipeline_cache.c
│   │   ├── queue_optimizer.c
│   │   ├── frame_pacing.c
│   │   └── cmd_buffer.c
│   └── wrapper/               ← Wrapper .so C source
│       ├── fexdxvk_wrapper.c
│       ├── gpu_detect.c
│       ├── thermal.c
│       ├── cpu_sched.c
│       ├── memory.c
│       └── monitor.c
└── init.sh                    ← Entry point (Winlator calls this)
```

---

## Target GPUs
Mali-G57 · Mali-G68 · Mali-G610 · Mali-G615 · Mali-G720 · PowerVR Series

**Not for Adreno GPUs.** Use the standard Winlator DXVK wrapper for Qualcomm devices.

---

## Profiles

| Profile | GPU Power | Thermal | Notes |
|---------|-----------|---------|-------|
| `balanced` | Balanced | 75°C | Default |
| `performance` | Performance | 85°C | Max FPS |
| `cool` | Cool | 65°C | Lower temps |

Edit `config/wrapper.json` → `"profile"` to switch.

## Release binary status

The release package includes the complete official DXVK 3.0.2 x32/x64 DLL sets,
the complete VKD3D-Proton 3.0.1 x86/x64 D3D12 DLL sets, and the two custom
Android arm64-v8a libraries built with NDK r25c.

FEX-Emu does not publish a prebuilt FEX-2607 Android/ARM64 runtime asset. The
`FEXInterpreter`, `FEXBash`, `libFEX.so`, and thunk libraries must be built on an
ARM64 Linux/Android build environment and copied into `prebuilt/fex/`. They are
not replaced with incompatible host binaries.
