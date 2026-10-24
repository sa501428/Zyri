# Zyri verification record

Local verification on October 22, 2026 used AppleClang 17, Qt 6.10.2, and an Apple silicon macOS host. Application code was compiled as C++20. Both build configurations use the included, hash-verified libzip 1.11.4 archive with no temporary source-directory override.

## Automated checks

The Editor CTest executable reports **50 passed, zero failed** in the normal Debug configuration and in the UndefinedBehaviorSanitizer configuration. The UBSan run uses `halt_on_error=1:print_stacktrace=1`.

Checks include alpha/blend reference values, independent layers, copy-on-write snapshots, transaction rollback, saved-state branching and asynchronous save completion, continuous strokes and stroke opacity, locks/bounds, selections and fill tolerance, structural undo, history spill, image/canvas transforms, native mixed-document round trips, invalid input, PNG/JPEG/PDF/SVG output, UI input, document tabs, recovery generations, and synthetic offscreen layout/icon rendering. Export coverage also checks that PNG uses effective compression without changing pixels or color metadata.

Quit regressions send native application Quit events to the same application class used by the executable. They verify Save, Discard, Cancel, repeated quit requests while the decision dialog is open, cancellation of a window close, and failed-save handling. Save must complete before closing; Cancel and write failures keep the edited document open. These tests use Qt's offscreen platform and do not interact with the personal desktop.

The address/undefined sanitizer configuration compiled during implementation, but AddressSanitizer stalled before main on this host. The same failure occurred in a minimal standalone probe. An ASan runtime pass is not claimed.

## App bundle and artifacts

The install path creates a self-contained development bundle and copies the actual Qt plugin binaries rather than package-manager symlinks. Deployment rejects tool errors and verifies the completed ad-hoc signature with `codesign --verify --deep --strict`.

The bundled executable generated native, PNG, SVG, and PDF fixtures using Qt's offscreen platform, with Qt plugin/development dynamic-library environment paths removed. A scan of 35 unique Mach-O files found no runtime dependency pointing into the development Homebrew tree, workspace, or temporary staging directories. Library identity records were distinguished from runtime dependencies.

The development ZIP passed `unzip -t`. Build outputs, the app, ZIP, and examples remain under the ignored build directory. They can be regenerated with the documented install and fixture commands.

The local bundle is **arm64, macOS 15 or later**, because the installed QtSvg binary requires macOS 15. The compiler target and bundle metadata match that dependency minimum.

## Limits of this verification

No personal desktop, screen capture, or accessibility interface was accessed. UI checks draw synthetic widgets directly to an offscreen image. The normal visible application, tablet hardware, platform gestures, clean machines, Windows/Linux CI jobs, production Developer ID signing, notarization, and the proposed performance scorecard have not been certified by these checks.

The implementation status document records remaining engineering and release work, including GPU presentation, performance under memory pressure, advanced text/path behavior, and format fidelity boundaries.
