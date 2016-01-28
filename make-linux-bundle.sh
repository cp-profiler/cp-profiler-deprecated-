# Run this from the build directory.

HASH=$(git show-ref -s HEAD)
SHORT=${HASH:0:8}
DATE=$(date +%Y%m%d)

make

DIRNAME=cp-profiler-$DATE-$SHORT
mkdir $DIRNAME

cp cp-profiler $DIRNAME
cp ../cp-profiler.sh $DIRNAME
cp /usr/lib/x86_64-linux-gnu/libQt5Gui.so.5 $DIRNAME
cp /usr/lib/x86_64-linux-gnu/libQt5Core.so.5 $DIRNAME
cp /usr/lib/x86_64-linux-gnu/libQt5Network.so.5 $DIRNAME
cp /usr/lib/x86_64-linux-gnu/libQt5PrintSupport.so.5 $DIRNAME
cp /usr/lib/x86_64-linux-gnu/libQt5Widgets.so.5 $DIRNAME
cp /usr/lib/x86_64-linux-gnu/libprotobuf.so.9 $DIRNAME

zip -r $DIRNAME.zip $DIRNAME
