# Validation record: park-explorer proof 0.1

## Completed and verified

| Check | Result |
|---|---|
| Unmodified upstream checkout | Commit `21afdbc55008335cd70e72a53d465608f9f645ac`; clean worktree before applying patch |
| Native upstream build | Full Release GUI + CLI build and generated graphics passed, GCC 13.3 / Ubuntu 24.04 x86_64 |
| Native patched build | Full GUI + CLI compiled and linked with upstream warning/error settings; final Explorer change rebuilt and linked |
| Source provenance | All changed `src/` files in the build checkout byte-compared with delivered source |
| Executable smoke checks | `--version` and `--help` run successfully; version invocation also run with handheld environment set (this does not initialise gameplay/UI) |
| SDL integration tests | CTest 1/1 passed; test executable uses actual SDL virtual controllers |
| Input cases exercised | No device, connect, mapped A/B, no held-A repetition, time-based D-pad movement, long-stall cap, opposite directions, focus loss/release rearming, discarded queued action, raw event suppression, wrong-instance suppression, mouse/keyboard passthrough, disconnect |
| Source formatting / patch | New C++ clang-format checked; `git diff --check` clean |
| Shell templates | `bash -n` / `sh -n` passed; no claim of device execution |

Exact native flags: `-G Ninja -DCMAKE_BUILD_TYPE=Release -DDISABLE_DISCORD_RPC=ON -DDISABLE_IPO=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`. CCache was enabled for the successful full native builds. Scripting, networking, HTTP, fonts, codecs and OpenGL were left at upstream defaults. No simulation feature was disabled.

The first configure/build attempts exposed missing local packages (including `nlohmann-json3-dev`); those were installed. The final successful build used upstream sources without compatibility patches for those dependencies. PNG profile warnings occurred during graphics generation and did not fail the build. No upstream gameplay tests or save-roundtrip tests were run.

The copied native executable's permission bit was restored before smoke checking and packaging. The archive records it as executable.

## Built but not exercised in a running park

- Context guards and action dispatch in the full application.
- Direct viewport pan, centre picking, camera zoom/rotation and pause.
- Overlay composition and small-screen rendering.
- Lowered SDL window minimum and integer-scale profile.
- Game-level behaviour with handheld mode disabled.

The SDL tests validate the transport module, not those full-game behaviours. Original RCT2 assets were not available in this environment, so no in-park runtime, screenshot, rendering or readability claim is made.

## Not built or tested

- AArch64 target executable / dependency bundle.
- NextUI / PortMaster launcher execution.
- TrimUI physical controls, display, performance, sleep/resume, save/load.
- Complete controller-only menus, management, construction or quit flow.

The environment has no validated TrimUI sysroot, AArch64 cross toolchain, container runtime, or attached Brick. Target packaging remains a gate, not a completed deliverable. The included CMake cross-toolchain file and launcher are proposals requiring the real SDK/runtime. The native archive is **x86_64 Linux only**, dynamically linked to the native dependencies listed in `evidence/native-dependencies.txt`; it is not a portable Linux release, macOS app or Brick installation.

## Binary identity

Baseline GUI SHA-256:

`54ee876a6e496596a15df5b86ebe12597451c45ade5fd1acd33fc892ece2e9e3`

Patched GUI SHA-256:

`eb49e62a3a110ef77bb186510cbbb24e36c3f6af488e854883dc043ec0b45def`

Both report the upstream v0.5.5 revision because the build checkout was patched on top of the pinned detached baseline. Use the hashes and delivered patch to distinguish them; the reported version alone does not identify this prototype. The patch does not change the network/plugin/save version.

`evidence/` includes build execution records, package versions, source hashes, executable hashes, CLI smoke output and the SDL test result. No remote GitHub fork/branch was created or pushed; development is preserved in the delivered source archive and patch. To continue, import the source or apply the patch to the pinned baseline in a user-owned fork.
