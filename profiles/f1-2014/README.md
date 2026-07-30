# FexDXVK F1 2014 — Cool High-Quality Profile

Game-specific Winlator GameHub package for **F1 2014**.

F1 2014 is normally a 32-bit Direct3D 9 game, so the primary runtime is the
DXVK `x32` directory. The package also carries the x64 DXVK files and VKD3D
files for wrapper layouts that expect both architectures.

## Goals

- Keep the game's high visual quality.
- Reduce sustained heat and avoid thermal throttling.
- Reduce shader-compilation spikes after the first launch.
- Avoid duplicate frame-pacing sleeps and unnecessary background polling.
- Prefer a stable 60 FPS cadence when the device can sustain it.

This is a cooler profile, not a guaranteed FPS or temperature result. The
ideal profile depends on the phone, ambient temperature, driver, resolution,
and game settings.

## Package contents

```text
config/
  f1-2014-dxvk.conf       DXVK 3.0.2 settings for F1 2014
  f1-2014-fex.conf        FEX-2607 ARM64 profile
  f1-2014-vkd3d.conf      VKD3D-Proton compatibility profile
  f1-2014-wrapper.json    Wrapper profile and feature flags
prebuilt/
  dxvk/x32/               32-bit DXVK DLLs used by F1 2014
  dxvk/x64/               64-bit DXVK DLLs
  vkd3d/x32/              32-bit VKD3D-Proton DLLs
  vkd3d/x64/              64-bit VKD3D-Proton DLLs
  wrapper/                Android arm64 wrapper library
  vulkan_layer/           Android arm64 Vulkan layer
  fex/                    FEX runtime slot; ARM64 build required
layer/                    Vulkan implicit layer manifest
init-f1-2014.sh           Game-specific launcher
```

## Import

1. Copy `FexDXVK-F1-2014-Cool.winlator` to Android.
2. Open Winlator GameHub.
3. Open **Wrappers → Import**.
4. Select the package.
5. Edit the F1 2014 container and select the imported wrapper.
6. Start with the game's native 1280x720 or 1920x1080 resolution.
7. Use the `balanced-cool` profile first.

For a cooler setup, use a 60 FPS in-game or container limit. Avoid running
uncapped if the device is already warm.

## FEX runtime requirement

This package includes the custom FEX configuration, but no fake host FEX
executables. FEX-Emu 2607 must be built on an ARM64 Android/Linux environment
and copied into:

```text
prebuilt/fex/FEXInterpreter
prebuilt/fex/FEXBash
prebuilt/fex/lib/libFEX.so
prebuilt/fex/lib/libFEXCore.so
prebuilt/fex/thunks/*.so
```

Verify every runtime file with `file` before importing it. Do not copy x86
host FEX binaries into this Android package.

## Launcher environment

`init-f1-2014.sh` sets:

- F1-specific DXVK, FEX, and VKD3D configuration paths.
- 32-bit DXVK/Vulkan DLL overrides.
- The ARM64 wrapper and Vulkan implicit layer.
- Low-overhead defaults: no worker pool, HUD, or custom present sleep.

For temporary diagnostics:

```bash
FEXDXVK_MONITOR=1 ./init-f1-2014.sh <wine-or-fex-command>
```

Do not leave the monitor enabled for normal play; it adds polling overhead.

## Troubleshooting

### F1 2014 does not start

- Confirm the game is using a 32-bit Wine prefix.
- Confirm `prebuilt/dxvk/x32/d3d9.dll` and `dxgi.dll` exist.
- Try disabling the implicit layer by removing `VK_INSTANCE_LAYERS` from the
  launcher to isolate a Vulkan-layer issue.
- Start with balanced graphics settings and then increase quality.

### Too hot

- Keep the 60 FPS cap enabled.
- Lower resolution before lowering texture quality.
- Change `thermalThresholdCelsius` from 70 to 68 in
  `config/f1-2014-wrapper.json`.
- Use the `cool` power profile if the device still throttles.

### Stutter after installing a new driver

The first run rebuilds shaders. Allow one complete lap, exit normally, and
launch again so the persistent DXVK state cache can be reused.

## Upstream components

- DXVK 3.0.2: <https://github.com/doitsujin/dxvk>
- VKD3D-Proton 3.0.1: <https://github.com/HansKristian-Work/vkd3d-proton>
- FEX-Emu 2607: <https://github.com/FEX-Emu/FEX>