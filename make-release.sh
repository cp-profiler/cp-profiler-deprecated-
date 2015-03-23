#!/bin/sh
set -e

cd /home/maxim/Dropbox/dev/phd/StandaloneGist/build-StandAloneGist-Desktop_Qt_5_3_GCC_64bit-Debug
qmake ../StandAloneGist.pro -r -spec linux-g++ CONFIG+=debug
make -j6 -Wall
mv StandAloneGist ../StandAloneGist-debug
cd ..

