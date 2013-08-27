#! /usr/bin/env bash
#
# Builds a zulip desktop app package for win32

if [ -z $1 ]
then
	echo "Please pass the version to package"
	exit 1
fi

cd /tmp
rm -rf zulip-desktop
git clone git@git.zulip.net:zulip-desktop.git
pushd zulip-desktop
pushd admin/win
popd
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../admin/win/Toolchain-mingw32-openSUSE.cmake -DCMAKE_BUILD_TYPE=Release ..
make -j3
make package

mv zulip-$1.exe ~/packages/

popd
rm -rf zulip-desktop/

echo "Generated zulip-$1.exe installer in ~/packages!"
