#!/bin/sh
# Run after staging a self-contained app with cmake --install.
set -eu
: "${ZYRI_SIGN_IDENTITY:?Set ZYRI_SIGN_IDENTITY to your Developer ID Application identity}"
: "${ZYRI_NOTARY_PROFILE:?Set ZYRI_NOTARY_PROFILE to an existing notarytool keychain profile}"
app_path=${1:?Usage: sign-and-notarize.sh /path/to/Zyri.app}
/usr/bin/find "$app_path/Contents" -type f \( -name '*.dylib' -o -name '*.so' \) -exec /usr/bin/codesign --force --timestamp --options runtime --sign "$ZYRI_SIGN_IDENTITY" {} \;
/usr/bin/find "$app_path/Contents/Frameworks" -depth -type d -name '*.framework' -exec /usr/bin/codesign --force --timestamp --options runtime --sign "$ZYRI_SIGN_IDENTITY" {} \;
/usr/bin/codesign --force --timestamp --options runtime --sign "$ZYRI_SIGN_IDENTITY" "$app_path"
/usr/bin/codesign --verify --deep --strict "$app_path"
archive_path="${app_path%.app}-notarization.zip"
/usr/bin/ditto -c -k --keepParent "$app_path" "$archive_path"
xcrun notarytool submit "$archive_path" --keychain-profile "$ZYRI_NOTARY_PROFILE" --wait
xcrun stapler staple "$app_path"
