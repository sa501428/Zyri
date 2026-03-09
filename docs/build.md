# Building Zyri

## Prerequisites

### macOS

Install dependencies via Homebrew:

```bash
brew install qt@6 lcms2 poppler cmake pkg-config
```

Qt is keg-only on Homebrew, so add it to your PATH:

```bash
echo 'export PATH="/opt/homebrew/opt/qt@6/bin:$PATH"' >> ~/.zshrc
echo 'export CMAKE_PREFIX_PATH="/opt/homebrew/opt/qt@6"' >> ~/.zshrc
source ~/.zshrc
```

### Windows

1. Install [Qt 6.7+](https://www.qt.io/download) with the MSVC 2022 kit
2. Install [CMake](https://cmake.org/download/) 3.25+
3. Install [vcpkg](https://vcpkg.io/) then:
   ```
   vcpkg install lcms2 poppler
   ```

### Linux (Ubuntu/Debian)

```bash
sudo apt install qt6-base-dev qt6-base-dev-tools libqt6opengl6-dev \
                 liblcms2-dev libpoppler-qt6-dev cmake pkg-config \
                 libgl-dev
```

---

## Build

```bash
# Clone (if you haven't already)
git clone https://github.com/yourusername/Zyri.git
cd Zyri

# Configure (macOS with Homebrew Qt)
cmake -B build -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_PREFIX_PATH="/opt/homebrew/opt/qt@6"

# Build
cmake --build build --parallel

# Run
./build/Zyri.app/Contents/MacOS/Zyri     # macOS
./build/Zyri                              # Linux/Windows
```

### Release build

```bash
cmake -B build-release -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH="/opt/homebrew/opt/qt@6"
cmake --build build-release --parallel
```

### macOS .dmg packaging

```bash
cmake --build build-release --target package
```

---

## Running Tests

```bash
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Or run individual test binaries directly:

```bash
./build/tests/ZyriCoreTests
./build/tests/ZyriHistoryTests
./build/tests/ZyriEngineTests
```

---

## Troubleshooting

**Qt not found:**
```
CMake Error: Could not find a package configuration file provided by "Qt6"
```
Add your Qt install to `CMAKE_PREFIX_PATH`:
```bash
cmake -B build -DCMAKE_PREFIX_PATH="/path/to/qt/6.x/macos"
```

**lcms2 not found:**
```bash
brew install lcms2   # macOS
sudo apt install liblcms2-dev   # Ubuntu
```

**poppler-qt6 not found:**
```bash
brew install poppler   # macOS
sudo apt install libpoppler-qt6-dev   # Ubuntu
```
