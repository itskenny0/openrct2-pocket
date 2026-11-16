# Phase 5 runtime UI captures

28 individual 512×384 PNGs plus a contact sheet. Native AArch64 runtime fixtures
under QEMU render the actual park and production UI using the user's private RCT2
assets. These are not physical Brick screenshots or generated mockups.

[Contact sheet](contact-sheet.png) · [Controls and verification](../../CONTROLLER-UX-PHASE5.md)

- 01–08: empty/populated/paged staff roster, hiring review, employee Overview,
  Duties draft, native patrol editing and safe dismissal review.
- 09–17: guest Overview, Needs, Thoughts, Details/spending/tracking, native follow,
  populated and 1,024-person roster pages.
- 18–24: shared-color path Build/Adjust, scenery Build/Adjust, landscaping
  Build/Adjust and Dashboard regression captures.
- regression-*: native Open/Closed ride states, demolition review and coaster
  construction from the passing RideFlowTests driver.

The optional bright-sand capture diagnostic faults after successful native terrain
repaint. Its capture is not included or claimed as verified. See the milestone
report and separate failure log. Headless focus-dependent brush targeting and
physical 2× display output remain outside this fixture's full coverage.
