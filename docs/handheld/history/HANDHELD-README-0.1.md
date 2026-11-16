# OpenRCT2 Handheld — park-explorer proof 0.1

This archive contains the full pinned upstream source plus a small opt-in controller layer, not a simplified RCT clone. Start with:

- `docs/handheld/RECONNAISSANCE.md`: source map, architecture, risks, ARM route and phased plan.
- `docs/handheld/BUILDING.md`: native build, input tests, proposed cross-build and hardware checklist.
- `docs/handheld/VALIDATION.md`: exact verified/unverified boundary and binary hashes.

Native upstream and patched Release builds passed on x86_64 Linux. SDL virtual-controller tests passed. The actual park explorer has **not** been exercised with original game assets or on TrimUI hardware. No AArch64 game binary is included.

Enable the prototype with `OPENRCT2_HANDHELD=1`. Its D-pad pans the map directly; A selects at a fixed reticle; B cancels/closes; X rotates; L/R zoom; Start pauses. Desktop UI, mouse and keyboard remain available. This is an incomplete Phase 1 proof: no controller-native main menu, file picker, management or construction yet.

The baseline is OpenRCT2 commit `21afdbc55008335cd70e72a53d465608f9f645ac`. Preserve this baseline when applying the companion patch. This source archive already has the patch applied. There are no proprietary RCT assets. OpenRCT2 and these modifications are licensed under GPL-3.0; retain the upstream license and third-party notices.
