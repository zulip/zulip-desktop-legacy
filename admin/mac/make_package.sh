#! /usr/bin/env bash
#
# Builds a zulip desktop app package for OS X

if [ -z $1 ]
then
	echo "Please pass the version to package"
	exit 1
fi

cd /tmp
rm -rf zulip-desktop
git clone git@git.zulip.net:eng/zulip-desktop.git
pushd zulip-desktop
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j3
../admin/mac/build-release-osx.sh "$@"

mv Zulip-$1.dmg ~/packages/
mv Zulip-$1.tar.bz2 ~/packages/sparkle

popd
rm -rf zulip-desktop/

echo "Generated DMG Zulip-$1.dmg in ~/packages!"
