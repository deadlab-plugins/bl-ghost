#!/bin/sh

CUR_DIR=$(pwd)

cd iPlug2
ln -s ../libs bl-libs

cd $CUR_DIR

cd iPlug2/Examples/bl-ghost
ln -s ../../bl-ghost bl-ghost
cd bl-ghost

make app
make vst2
make vst3

cd bl-ghost/installer
install-plug.sh
install-app.sh

cd CUR_DIR
