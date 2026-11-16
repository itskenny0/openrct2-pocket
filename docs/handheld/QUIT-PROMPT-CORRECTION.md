> Follow-up: the complete controller quit routing, pause/speed controls and path fixes are now implemented and native-tested in [EVERYDAY-CONTROLS.md](EVERYDAY-CONTROLS.md). The source-only limits below describe the earlier 81bc3bb checkpoint.

# Pocket quit prompt correction — source checkpoint

Scope: the two confirmed Pocket exits in `src/openrct2-ui/handheld/Menu.cpp` only.
This is an incomplete checkpoint for Path Reliability, Safe Quitting & Everyday Controls,
not a Brick release or completion of that milestone.

## Cause and change

Both successful Save and exit and the final Exit WITHOUT saving action called
`ContextQuit()`. `Context::Quit()` sets `gSavePromptMode` to quit and opens the
native desktop save prompt. This introduces a second prompt after Pocket has
already handled the decision, matching the reported inaccessible dialog.

Those two sites now call `GetContext()->Finish()` after closing the Pocket panel.
`Context::Finish()` sets the loop completion flag, allowing normal loop return
and Context cleanup. It does not terminate the process directly. The successful
save path still waits for the synchronous atomic save result and updates the
loaded/save path metadata before finishing. Save failure and stale destination
return before the finish call. Cancel/Back handlers and focus restoration were
not changed.

## Verification and limits

- Reviewed current canonical Menu.cpp, Context.cpp, Context.h, SavePrompt.cpp,
  Explorer.cpp and PauseResumeTests.cpp at dc4d8c64013beaa0ba612468f45e94100b1bc8ae.
- Source routing assertions pass: no ContextQuit calls remain in Menu.cpp;
  both final exit paths call Finish; failed-save handling precedes completion.
- These are source checks, not native regression tests. No full application
  compilation, native controller run, ARM/QEMU check or physical Brick test
  was completed for this checkpoint.
- Package recovery helper returned HTTP 403. Git clone lacked authentication.
  A public upstream archive was retrieved, but complete fork source recovery
  and build environment setup were not completed. Package installation failed
  because the system package cache was not writable.
- No installable update, replacement executable or new runtime screenshot was
  produced. Keep the installed UI Audit + Crash Close baseline for now.

## Remaining work

- Compile and validate both changed paths, including failed/cancelled saves,
  overwrite cancellation, caller focus and normal cleanup/NextUI return.
- Route other quit entry points (including Explorer's quit chord and native quit
  requests) through controller-native confirmation. They still call ContextQuit
  and are not fixed by this checkpoint.
- Complete the requested Save and quit / Quit without saving / Cancel flow.
- Investigate and fix Single/Line/Fill height, real placement, money,
  connectivity and path additions. No path behavior changed here.
- Implement and validate Paused / 1x / 2x / 4x with pause ownership semantics.
- Complete focused player-facing information, native runtime/controller tests,
  AArch64 packaging, durable deliverables and runtime screenshots.

