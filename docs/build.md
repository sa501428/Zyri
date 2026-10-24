# Building Zyri

## Dependencies

Use CMake 3.25+, a C++20 compiler, and Qt 6.8 or later with Core, Gui, Widgets, Concurrent, and Svg. Qt Pdf enables PDF import and is on by default; disable with `-DZYRI_PDF=OFF`. PDF export uses Qt Gui and remains available.

The tested local baseline is AppleClang 17 and Qt 6.10.2 on macOS. The deployment target is raised to the minimum of the linked SDK modules when necessary. This Homebrew SDK includes QtSvg built for macOS 15, so its bundled app requires macOS 15 or later; an older compatible SDK can support an older deployment target. Objective-C++ and OpenGL are not required by application code.

CMake first looks for an installed libzip CMake package. If unavailable, FetchContent extracts the included libzip 1.11.4 source archive and verifies its SHA-256. Only the ZIP library and zlib compression are needed; tools, optional crypto/compression dependencies, and examples are disabled.

The default fallback is offline. You can also supply an existing libzip installation via CMAKE_PREFIX_PATH, or an unpacked source tree:

```sh
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH="/path/to/Qt;/path/to/dependencies" \
  -DFETCHCONTENT_SOURCE_DIR_LIBZIP=/path/to/libzip-1.11.4 \
  -DCMAKE_BUILD_TYPE=Debug
```

The source archive came from the official libzip release site; its license and provenance are under third_party. zlib must be supplied by the platform or dependency manager. The Windows CI uses the static MD vcpkg triplet.

## Configure and build

On macOS with Homebrew Qt:

```sh
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH="/opt/homebrew/opt/qt@6;/opt/homebrew" \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 6
ctest --test-dir build --output-on-failure
```

All registered tests use Qt's offscreen platform. They create synthetic documents, widgets, and files; they do not inspect the user's desktop.

With Ninja installed, the equivalent presets are:

```sh
cmake --preset debug -DCMAKE_PREFIX_PATH=/path/to/Qt
cmake --build --preset debug
ctest --preset debug
```

On Windows, use a Qt installation matching the compiler and architecture, with an appropriate CMAKE_PREFIX_PATH. CMake's generated deployment script handles the platform-specific Qt dependency deployment. The CI workflow includes macOS, Ubuntu, and Windows jobs; only the local macOS run has been observed in this development session.

## Sanitizers

```sh
cmake -S . -B build-sanitized \
  -DCMAKE_PREFIX_PATH="/path/to/Qt" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DZYRI_SANITIZERS=ON
cmake --build build-sanitized --parallel 6
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build-sanitized --output-on-failure
```

The address/undefined-behavior flags propagate through the actual application/library/test targets. Leak detection is disabled in the macOS test command because platform Qt/font allocations are outside the editor's ownership. Do not infer performance from a sanitized build.

The local AppleClang 17 / macOS 26.6.2 AddressSanitizer runtime stalled before main, including for a minimal standalone probe. A UBSan-only build was validated using `-DZYRI_SANITIZER_SET=undefined`. AddressSanitizer compilation succeeded, but an ASan runtime pass is not claimed. The CI configuration uses UBSan on macOS and address/undefined sanitizers on Linux.

Use BUILD_TESTING=OFF for a build without the test executable. It then does not require Qt Test.

## Run or generate fixtures

The macOS executable is `build/Zyri.app/Contents/MacOS/Zyri`. On other desktop platforms, use the corresponding executable under the build directory.

```sh
QT_QPA_PLATFORM=offscreen build/Zyri.app/Contents/MacOS/Zyri \
  --render-demo /tmp/zyri-demo
```

The fixture command produces `demo.zyri`, `demo.png`, `demo.svg`, and `demo.pdf`. It exercises painting, vector shapes, text, native persistence, and exports without showing a window.

## Package

```sh
cmake -S . -B build-release \
  -DCMAKE_PREFIX_PATH="/path/to/Qt" \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build-release --parallel 6
cmake --install build-release --prefix /tmp/zyri-staged
cpack --config build-release/CPackConfig.cmake
```

On macOS the install step uses Qt's deployment tools and explicitly selects the Cocoa/offscreen, JPEG, and native-style plugins. This avoids bundling unrelated virtual-keyboard/QML plugins from aggregate Qt installations. CPack selects a DMG on macOS, ZIP on Windows, and TGZ on Linux.

The direct-distribution signing helper at `packaging/macos/sign-and-notarize.sh` requires the developer's own signing identity and an existing notarytool keychain profile. It does not provision credentials. Review and run it on the staged app when preparing a release; signing/notarization were not performed in this session.

## Development layout

- ZyriModel: document/value types, CPU renderer, transactions/history, persistence.
- ZyriDesktop: widget canvas, input, panels, menus, background job/recovery coordination.
- TestEditor: behavioral QtTest suite registered with root CTest.
- Zyri: main executable and command-line fixture mode.

There are no required lcms2, Poppler, private Qt ZIP, or private GPU headers in the current implementation.
