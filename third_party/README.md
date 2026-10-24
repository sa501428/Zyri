# Bundled libzip source

The fallback dependency archive is the unmodified libzip 1.11.4 source release, downloaded from https://libzip.org/download/libzip-1.11.4.tar.xz.

SHA-256: `8a247f57d1e3e6f6d11413b12a6f28a9d388de110adc0ec608d893180ed7097b`.

The upstream license is copied to `licenses/libzip.txt` and is also inside the archive. CMake prefers an installed libzip 1.11+ package, otherwise verifies and extracts this archive inside the build directory. No dependency download is required for this fallback.

When updating, replace the archive, hash, and license together and rerun project, corruption, and recovery tests. Qt and zlib remain platform dependencies.
