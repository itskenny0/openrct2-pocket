# Brick release: shared settings sheet and real-park captures

The previous scenery/landscape settings capture had full-width tabs crossing
between two separately framed panels. The shared tab bar now belongs to one
continuous sheet. Its two content columns have a divider below the tabs;
there are no panel edges or grass gaps running through the tab strip.
Other management pages retain their own existing frames.

Validated at native 512×384 using FriendlyUiTests (controller action dispatch,
group boundaries, group focus restoration, disabled controls, material-picker
return, native scenery height, track connections and technical-details return).
The corrected scenery settings PNG was opened and visually inspected.

AArch64 packaging, runtime closure and real-park capture results are recorded
below when completed. Physical Brick testing is performed by the player;
host captures are not device captures.

## Completed release verification

Executable source: `8aa63aee5ca5da02581ae38d1ce726c67de850a7`.
The source checkpoint was verified against the canonical GitHub tree: 1,801
source, CMake, handheld test and packaging files matched their Git blob hashes.
The following release notes/capture driver do not change executable source.

The focused native Linux runtime suites passed: FriendlyUiTests,
EverydayControlsTests, SceneryPathParityTests, DesignSubstitutionTests and
SelectedTrackEditingTests. All nine standalone CTest checks passed. Everyday
controls includes mapped SDL virtual-controller events; the capture driver
uses production controller action dispatch plus native page entry points.

RealParkCaptures loads the supplied Six Flags Holland scenario, with 19 rides
and 1,331 guests at scenario start. It captures actual native park graphics,
checks Select opens Build, and captures native scenery previews from the
scenario's loaded/invented object list. The selected six images cover Goliath,
Robin Hood, Build, the scenery catalog, a ride overview and corrected settings.
Original 512×384 PNGs and exact nearest-neighbour 1024×768 exports are supplied.
These are Linux native runtime frames, not mockups or physical Brick photos.

AArch64 GUI and CLI: GCC 16.2, Cortex-A53, Release O3/NDEBUG, original private
glibc 2.26 / vendor SDL 2.26.1 runtime. Both SDK/compiler archives match BUILD.md
SHA-256 values. zstd 1.5.7 and libzip 1.11.3 were built using the private SDK;
ICU 76.1 headers came from its source release and linked against the unchanged
compatible ICU binaries in the installed package. All runtime libraries remain
unchanged. The private build-time RPATH was removed when linking delivery
binaries; the launcher continues selecting its own private runtime.

Both stripped binaries pass QEMU 7.2 AArch64 version execution with the original
private loader/libraries, reporting v0.5.5 / 8aa63ae / AArch64. This tests runtime
loading, not ARM gameplay. Generated g2/fonts/palettes/tracks match the installed
UI Audit package byte for byte. Network, HTTP, OpenGL, TTF, Vorbis and FLAC
remain disabled; scripting remains enabled, matching the existing target build.

No physical Brick is connected. GPU/display/audio, sleep/resume, SD-card saving
and dense-park performance still require physical testing. Prior user reports
of stable play refer to their earlier installed build, not this new binary.

The merged NextUI package passes the existing verifier: CRC, safe paths,
executable modes, complete manifest hashes, AArch64 ELF/dependency closure,
resources, launcher isolation and GUI/CLI QEMU version checks. The delivered
merge-only ZIP contains the two executables, manifest, checksums, cumulative
source patch, provenance and install/validation documents. Its contents and CRC
were checked; it contains no assets, user settings/saves, libraries or launcher.
The cumulative patch was applied to the original UI Audit source ZIP and all
changed files reproduced the current source bytes.

Install: quit the game, back up the two existing executables and manifest,
extract `OpenRCT2-Brick-Daylight-20261021.zip` onto the SD-card root and merge
only its included files. Never delete the existing .pak directory. Preserve
`.userdata/tg5040/openrct2-pocket` and all original assets. Restore the backed-up
executables/manifest to roll back.
