#! /usr/bin/env bash
#
# Builds a zulip desktop app package for win32

if [ -z "$1" ]
then
	echo "Please pass the version to package"
	exit 1
fi

if [ -n "$2" ]
then
	echo "Building SSO package"
	ENABLE_SSO=true
        SSO_CMD="-DSSO_BUILD=ON"
fi

cd /tmp
rm -rf zulip-desktop
git clone git@git.zulip.net:eng/zulip-desktop.git
pushd zulip-desktop
pushd admin/win
popd
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../admin/win/Toolchain-mingw32-openSUSE.cmake -DCMAKE_BUILD_TYPE=Release "$SSO_CMD" ..

make -j3

signcode -spc ~/real_windows_key.spc -v ~/real_windows_key.pvk -a sha1 -$ commercial -n Zulip -i https://zulip.com \
  -t http://timestamp.verisign.com/scripts/timstamp.dll -tr 10 zulip.exe < ~/windows_signing_passphrase.private

make package

zulip_bin="zulip-$1.exe"
if [ -n "$ENABLE_SSO" ]
then
        mv "$zulip_bin" "zulip-$1-sso.exe"
        zulip_bin="zulip-$1-sso.exe"
fi

signcode -spc ~/real_windows_key.spc -v ~/real_windows_key.pvk -a sha1 -$ commercial -n Zulip -i https://zulip.com \
  -t http://timestamp.verisign.com/scripts/timstamp.dll -tr 10 "$zulip_bin" < ~/windows_signing_passphrase.private

mv "$zulip_bin" ~/packages/

popd
rm -rf zulip-desktop/

echo "Generated $zulip_bin installer in ~/packages!"
