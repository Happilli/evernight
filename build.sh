#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

MODE="${1:-rel}"
QTCMAKE=~/Qt/6.12.0/android_arm64_v8a/bin/qt-cmake
COMMON=(-G Ninja -DQT_HOST_PATH=$HOME/Qt/6.12.0/gcc_64 -DANDROID_SDK_ROOT=$ANDROID_SDK_ROOT -DANDROID_NDK_ROOT=$ANDROID_NDK_ROOT -DCMAKE_EXPORT_COMPILE_COMMANDS=ON)

if [[ "$MODE" == "dev" ]]; then
  [[ -f build/build.ninja ]] || $QTCMAKE -S . -B build "${COMMON[@]}" -DCMAKE_BUILD_TYPE=Debug
  cmake --build build --target apk
  APK_OUT=build/android-build/build/outputs/apk/debug/android-build-debug.apk
else
  [[ -f build-rel/build.ninja ]] || $QTCMAKE -S . -B build-rel "${COMMON[@]}" -DCMAKE_BUILD_TYPE=Release
  cmake --build build-rel --target apk
  BT=$(ls -d "$ANDROID_SDK_ROOT"/build-tools/* | sort -V | tail -1)
  APK=build-rel/android-build/build/outputs/apk/release
  "$BT/zipalign" -f -p 4 "$APK/android-build-release-unsigned.apk" /tmp/evernight-aligned.apk
  "$BT/apksigner" sign --ks ~/.android/debug.keystore --ks-pass pass:android --out /tmp/evernight-rel.apk /tmp/evernight-aligned.apk
  APK_OUT=/tmp/evernight-rel.apk
fi

adb install -r "$APK_OUT"
adb shell monkey -p com.safalski.evernight -c android.intent.category.LAUNCHER 1
