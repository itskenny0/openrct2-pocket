# Brick runtime recovery — first hardware launch

The hardware trace on 2026-10-14 demonstrates that the native game executable,
controller enumeration and 1024x768 display initialization run, then SDL window
creation fails. The packaged private runtime is glibc 2.26. SDL/native PowerVR
GLES libraries dynamically load system librt.so.1 requiring GLIBC_2.32; native
ALSA loads system libasound.so.2 requiring GLIBC_2.29 from libm. These cannot
resolve against the private 2.26 core. This was a packaging omission: executable
DT_NEEDED traversal alone does not cover SDL/device dlopen dependencies.

The package helper now includes matching compiler-sysroot librt.so.1 and the
official TrimUI SDK ALSA 1.1.4.1 libasound.so.2, then recursively audits their
DT_NEEDED dependencies. SDK setup includes this ALSA binary. Native device
GPU libraries remain device-owned and are not redistributed. No core system
libraries, gameplay state, assets or saves are modified.

The small OpenRCT2-Pocket-Brick-Runtime-Fix.zip supplies those two AArch64
libraries for the existing game/lib directory. ELF architecture/version needs
were inspected. Both library dependency graphs resolve successfully under
QEMU 7.2 with the exact glibc 2.26 loader/core used for the compiled package.
ALSA requires only GLIBC_2.17. The GCC archive SHA256 matches the original
verified compiler archive. These are loader checks, not physical graphics,
audio, gameplay or save validation. Hardware retest remains required.

Separate filesystem corruption was demonstrated by macOS fsck_exfat after
files disappeared across a boot-only cycle. The user performed a fresh setup
and restored the executable. Do not conflate this with the graphics runtime
failure; neither the dock nor card hardware has been established as its cause.

Keep the diagnostic layout-detecting launch.sh during the next hardware test;
it supports game/ and direct package layouts and records loader diagnostics.
The installed original testing ZIP still requires this runtime patch; a full
replacement release has not been generated. Further SDL udev fallback warnings,
ALSA device/plugin configuration and GPU initialization require hardware evidence
before additional changes. Original assets are supplied by the user.
