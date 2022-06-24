#!/bin/sh

CUR_DIR=$(pwd)

# dependencies

# libflac
cd $CUR_DIR/libs
if [ ! -d "flac" ]; then
    git clone https://github.com/xiph/flac.git
    #git checkout a2fe43f64e7ce8057fb274c64996568d69b301b8
fi

cd flac
if [ ! -d "cmake-build" ]; then
    mkdir cmake-build
fi
cd cmake-build
cmake .. -DWITH_OGG=0
make

cd $CUR_DIR/libs
if [ ! -d "libsndfile" ]; then
    git clone https://github.com/libsndfile/libsndfile.git
fi

# libsndfile

cd libsndfile
#git checkout 77acc6d3
cd $CUR_DIR/libs
if [ ! -f "./libsndfile/configure" ]; then
    cd libsndfile
    ./autogen.sh
    ./configure

    cd $CUR_DIR/libs
    
    # apply flac patch
    patch -s -p0 -f < libsndfile-flac.patch
fi
#make

# iPlug2

# won't work for the moment, will need special version of nanovg etc...
#cd $CUR_DIR/libs
#git clone https://github.com/nd-bl/iPlug2.git
#git checkout linux-bl-fix

cd $CUR_DIR

if [ ! -d "iPlug2/bl-libs" ]; then
    cd $CUR_DIR/iPlug2
    ln -s ../libs bl-libs
fi

cd $CUR_DIR
if [ ! -d "iPlug2/Examples/bl-ghost" ]; then
    cd $CUR_DIR/iPlug2/Examples
    ln -s ../../bl-ghost bl-ghost
fi

cd $CUR_DIR/iPlug2/Examples/bl-ghost

# custom NanoVG
cd $CUR_DIR
if [ ! -d "nanovg" ]; then
    git clone https://github.com/nd-bl/nanovg.git
    cd nanovg
    git pull origin ndbl-branch
    git checkout ndbl-branch
    cd ..
    mv iPlug2/Dependencies/IGraphics/NanoVG iPlug2/Dependencies/IGraphics/NanoVG-old
    cp -R nanovg iPlug2/Dependencies/IGraphics/NanoVG
fi

make app
make vst2
make vst3

cd build-linux

../installer/install-plug.sh
../installer/install-app.sh

cd $CUR_DIR
