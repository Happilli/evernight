#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

[[ -f build/build.ninja ]] || ~/Qt/6.12.0/android_arm64_v8a/bin/qt-cmake -S . -B build -G Ninja -DQT_HOST_PATH=$HOME/Qt/6.12.0/gcc_64 -DANDROID_SDK_ROOT=$ANDROID_SDK_ROOT -DANDROID_NDK_ROOT=$ANDROID_NDK_ROOT

cmake --build build --target apk
adb install -r build/android-build/build/outputs/apk/debug/android-build-debug.apk
adb shell monkey -p com.safalski.evernight -c android.intent.category.LAUNCHER 1
