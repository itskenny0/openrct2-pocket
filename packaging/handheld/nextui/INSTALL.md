# OpenRCT2 Pocket: NextUI Brick release candidate

1. For an update, rename the existing package's `game` folder to an unused
   backup name such as `game-before-rc3`. Then copy the new ZIP's `game` folder
   into the package. Copy the new `launch.sh`, instructions and metadata too.
   Keep the package's existing `RCT2` folder and the SD card's `.userdata` folder.
   Use a fresh `game` folder, rather than merging old runtime files.
   For a first installation, extract the ZIP onto the SD card root. The result must be:
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

The prior RC1 plus the complete runtime replacement was reported launching and
playing well on a Brick. RC3 adds visual building choices, path state/material
panels, guided flat-ride placement and scenery/terrain color palettes. It has
native desktop interaction and ARM package checks, but still needs a Brick retest.
Audio, every button, suspend/resume, SD-card save round trips and performance
remain physical-device checks. Back up saves.

The final footer is single-row. Start opens the park menu; its Controls entry
shows the shortcuts from menu entry. See `manifest.json` for source/build identity.

Title sequences belong in `game/data/sequence/` (the engine lookup path).
`game/data/title/` was the incorrect RC1 package destination and caused the
blank startup background. RC2 validates the default sequence's archive and parks.

To check the files actually copied to the SD card on macOS or Linux, open
Terminal in the new `game` folder and run `shasum -a 256 -c SHA256SUMS.txt`.
Every entry should report OK. This checks the transferred files, not just the ZIP.
