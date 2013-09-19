#!/bin/bash
#
# Usage: ./admin/mac/build-release-osx.sh [--no-clean]
#
################################################################################


function header {
    echo -e "\033[0;34m==>\033[0;0;1m $1 \033[0;0m"
}

function die {
    exit_code=$?
    echo $1
    exit $exit_code
}
################################################################################

if [ -z $1 ]
then
    echo This script expects the version number as a parameter, e.g. 1.0.0
    exit 1
fi

ROOT=`pwd`
VERSION=$1

################################################################################

    mv zulip.app Zulip.app
    mv Zulip.app/Contents/MacOS/zulip Zulip.app/Contents/MacOS/Zulip

    header "Fixing and copying libraries"
    $ROOT/../admin/mac/macdeploy.py Zulip.app quiet

    cd Zulip.app

    header "Renaming icon"
    cp $ROOT/../admin/mac/qt.conf Contents/Resources/qt.conf

    header "Copying Sparkle and Growl frameworks"
    cp -R /Library/Frameworks/Sparkle.framework Contents/Frameworks
    cp -R /Library/Frameworks/Growl.framework Contents/Frameworks

    cd ..

    header "Copying launch helper app to main app bundle"
    codesign -s "Developer ID Application: Zulip, Inc" -f -v ./ZulipAppHelper.app

    mkdir -p Zulip.app/Contents/Library/LoginItems
    cp -R ZulipAppHelper.app Zulip.app/Contents/Library/LoginItems/


    header "Signing bundle"
    codesign -s "Developer ID Application: Zulip, Inc" -f -v ./Zulip.app

    header "Creating DMG"
    $ROOT/../admin/mac/create-dmg.sh Zulip.app
    mv Zulip.dmg Zulip-$VERSION.dmg

    header "Creating signed Sparkle update"
    $ROOT/../admin/mac/sign_bundle.rb $VERSION ~/Documents/humbug/sparkle_privkey.pem
    mv Zulip.app zulip.app

    header "Done!"
