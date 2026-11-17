# Contributing to OpenRCT2 Pocket

Thanks for helping! Pocket is a handheld front end built on [OpenRCT2](https://github.com/OpenRCT2/OpenRCT2).
The simulation, renderer and save format are OpenRCT2's; Pocket changes presentation and input.

## Reporting problems

- **Bugs:** use the *Bug report* issue form. Say which release or testing build you're on and which device.
- **Things that feel wrong:** use the *Feels wrong* form. Awkward controls and confusing screens count, even if
  nothing is technically broken.
- **Problems that also happen in desktop OpenRCT2** belong [upstream](https://github.com/OpenRCT2/OpenRCT2/issues).

Never attach or commit RollerCoaster Tycoon 2 game files.

## Code

- The handheld layer lives in [`src/openrct2-ui/handheld/`](src/openrct2-ui/handheld). Keep changes to upstream files to
  small hooks; that is what keeps merging OpenRCT2 releases manageable.
- Build and test as described in [docs/handheld/BUILDING.md](docs/handheld/BUILDING.md). Before opening a pull request:
  - run `scripts/check-code-formatting` (clang-format 20);
  - run the standalone tests (`ctest --test-dir build-input-tests`);
  - run the in-game drivers for the areas you touched (`scripts/handheld/verify-ui-redesign.py`).
- Every pull request gets a testing Pak from CI, so changes can be tried on a device before merging.

## Upstream

OpenRCT2 releases are merged in on their own branch (`upstream/vX.Y.Z`) and go through the same checks. If you find a
bug in OpenRCT2's own code, please fix it upstream too.

## Licence

Contributions are licensed under the GNU GPL v3 or later, the same as OpenRCT2 (see [licence.txt](licence.txt)).
