#! /usr/bin/env bash
#
# Builds a zulip desktop app package for OS X
# Call it like this: make_package.sh 0.5.0 "Zulip, Inc"
# Where the first argument is the name to sign with, and the second is the version

set -xe;

if [ -z "$1" ]
then
	echo "Please pass the version to package"
	exit 1
fi

if [ -n "$3" ]
then
	echo "Starting SSO build"
        ENABLE_SSO=1
        SSO_CMD="-DSSO_BUILD=ON"
else
	echo "Starting non-SSO build"
fi


rm -rf /tmp/zulip-desktop

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
cp -a $DIR/../.. /tmp/zulip-desktop
rm -rf /tmp/zulip-desktop/build
cd /tmp

pushd zulip-desktop
mkdir build
cd build
export MACOSX_DEPLOYMENT_TARGET=10.6

cmake -DCMAKE_BUILD_TYPE=Release "$SSO_CMD"  ..
make -j3
../admin/mac/build-release-osx.sh "$@"

zulip_bin="Zulip-$1.dmg"
zulip_sparkle_bin="Zulip-$1.tar.bz2"
if [ -n "$ENABLE_SSO" ]
then
	echo "Making SSO packages"
        mv "$zulip_bin" "Zulip-$1-sso.dmg"
        zulip_bin="Zulip-$1-sso.dmg"

        mv "$zulip_sparkle_bin" "Zulip-$1-sso.tar.bz2"
        zulip_sparkle_bin="Zulip-$1-sso.tar.bz2"
fi

mv "$zulip_bin" ~/packages/
mv "$zulip_sparkle_bin" ~/packages/sparkle
mv signature.raw ~/packages/sparkle/$zulip_sparkle_bin.sig

popd

echo "Generated DMG $zulip_sparkle and $zulip_sparkle_bin in ~/packages!"
