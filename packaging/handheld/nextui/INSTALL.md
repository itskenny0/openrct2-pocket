# OpenRCT2 Pocket: NextUI Brick release candidate

1. Extract the ZIP onto the SD card root. The result must be:
   `Tools/tg5040/OpenRCT2 Pocket.pak/launch.sh`.
2. Copy your original RollerCoaster Tycoon 2 `Data`, `ObjData`, and `Scenarios`
   folders into `Tools/tg5040/OpenRCT2 Pocket.pak/RCT2/`.
   `RCT2/Data/g1.dat` must exist. Preserve the original files and names.
   A complete legitimately obtained RCT2 installation is recommended; its
   original sound/music data remain necessary for original audio.
3. Safely eject the SD card, insert it, and select **OpenRCT2 Pocket** in
   NextUI's **Tools** menu. No mouse mapper or PortMaster installation is needed.
4. Start with a small native scenario. Save manually before experimenting.

Native user data, configuration, saves and autosaves persist at:
`.userdata/tg5040/openrct2-pocket/` on the SD card. Native parks are under its
`save/` directory. Updates do not overwrite `config.ini` or delete existing parks.
Older `save-handheld` profiles are not automatically migrated or deleted.

Use the native Pocket park menu's Save and Exit workflow. Do not use the
Start+Select emergency quit chord for ordinary sessions: it is a quit shortcut,
not a substitute for saving. Normal application exit returns to NextUI.

Launch diagnostics are appended to:
`.userdata/tg5040/openrct2-pocket/launch.log`.
If launch fails or controls are wrong, send that log and your NextUI/firmware
version. The log records joystick GUID/name and paths. Missing assets stop with a
clear logged message. The mapping uses the first built-in joystick and documented
NextUI tg5040 controls; Bluetooth/USB enumeration and alternate firmware require
manual checking. `OPENRCT2_BRICK_KEEP_MAPPING=1` preserves an existing supplied SDL
mapping. `OPENRCT2_RCT2_PATH` can select another original installation directory.

This is a real AArch64 native-engine testing build, not a physical-device-verified
release. GPU/display/audio, every physical button, save/resume on your SD card,
sleep behavior and performance still need your first Brick test. Back up saves.

The final footer is single-row. Start opens the park menu; its Controls entry
shows the shortcuts from menu entry. See `manifest.json` for source/build identity.
