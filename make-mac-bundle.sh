# Run this from the build directory.

HASH=$(git show-ref -s HEAD)
SHORT=${HASH:0:8}

make
macdeployqt cp-profiler.app

zip -r cp-profiler-$SHORT.zip cp-profiler.app
