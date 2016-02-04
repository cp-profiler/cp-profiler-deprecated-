# Run this from the build directory.

HASH=$(git rev-parse HEAD)
SHORT=${HASH:0:8}
DATE=$(date +%Y%m%d)
NAME=cp-profiler-mac-$DATE-$SHORT

make
macdeployqt cp-profiler.app
mv cp-profiler.app $NAME.app

zip -r $NAME.zip $NAME.app
