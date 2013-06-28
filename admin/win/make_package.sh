#! /usr/bin/env bash
#
# Builds a humbug desktop app package for win32

if [ -z $1 ]
then
	echo "Please pass the version to package"
	exit 1
fi

cd /tmp
rm -rf humbug-desktop
git clone humbug@git.humbughq.com:/srv/git/humbug-desktop.git
pushd humbug-desktop
pushd admin/win
sh update-vlc.sh
popd
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../admin/win/Toolchain-mingw32-openSUSE.cmake -DCMAKE_BUILD_TYPE=Release ..
make -j3
make package

mv humbug-$1.exe ~/packages/

popd
rm -rf humbug-desktop/

echo "Generated Humbug-$1.exe installer in ~/packages!"
