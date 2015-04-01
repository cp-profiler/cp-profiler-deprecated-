#!/bin/sh
set -e

cd build
qmake ../StandAloneGist.pro -r -spec linux-g++ CONFIG+=release
make -j6 -Wall
mv StandAloneGist ../StandAloneGist-release
cd ..

