#! /usr/bin/env bash
#
# Builds a humbug desktop app package for OS X

if [ -z $1 ]
then
	echo "Please pass the version to package"
	exit 1
fi

cd /tmp
rm -rf humbug-desktop
git clone git@git.humbughq.com:humbug-desktop.git
pushd humbug-desktop
mkdir build
cd build
cmake -DPHONON_INCLUDE_DIR=/usr/local/Cellar/kde-phonon/4.6.0/include/ -DPHONON_LIBRARY=/usr/local/Cellar/kde-phonon/4.6.0/lib/libphonon.4.6.0.dylib  -DCMAKE_BUILD_TYPE=Release ..
make -j3
../admin/mac/build-release-osx.sh "$@"

mv Humbug-$1.dmg ~/packages/
mv Humbug-$1.tar.bz2 ~/packages/sparkle

popd
rm -rf humbug-desktop/

echo "Generated DMG Humbug-$1.dmg in ~/packages!"
