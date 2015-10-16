#!/bin/bash
set -ex

# Sign Zulip.app on the host 10.9+ machine by SCPing the app bundle
# over, doing the signing, then SCPing it back
#
# NOTE: this requires the environment variable ZULIP_KEYCHAIN_PW to contain the password to
# unlock the Zulip_Signing keychain
#
HOST_SIGNING_MACHINE=172.16.205.1
HOST_USERNAME=humbug
VM_IP=172.16.205.129
VM_USERNAME=leo

tar cvfj /tmp/zulip-remotesigning.tar.bz2 "./$2"
scp /tmp/zulip-remotesigning.tar.bz2 $HOST_USERNAME@$HOST_SIGNING_MACHINE:/tmp/

#ssh -2 192.168.0.104 \
ssh -2  $HOST_USERNAME@$HOST_SIGNING_MACHINE \
   "set -ex;
    cd /tmp ;
    rm -Rf remote_signing ;
    mkdir remote_signing ;
    cd remote_signing ;
    tar xvfj /tmp/zulip-remotesigning.tar.bz2 ;
    security unlock-keychain -p $ZULIP_KEYCHAIN_PW ~/Library/Keychains/Zulip_Signing.keychain ;
    security list-keychains -s ~/Library/Keychains/Zulip_Signing.keychain
    codesign -s 'Developer ID Application: $1' -f -v --deep './$2'/Contents/Frameworks/*.framework/Resources/*.app ;
    codesign -s 'Developer ID Application: $1' -f -v --deep './$2'/Contents/Frameworks/*.framework ;
    codesign -s 'Developer ID Application: $1' -f -v --deep './$2'/Contents/Frameworks/*.dylib ;
    codesign -s 'Developer ID Application: $1' -f -v --deep './$2'/Contents/Library/LoginItems/*.app ;
    codesign -s 'Developer ID Application: $1' -f -v --deep './$2'/Contents/MacOS/* ;
    codesign -s 'Developer ID Application: $1' -f -v --deep './$2'/Contents/qt-plugins/crypto/*.dylib ;
    codesign -s 'Developer ID Application: $1' -f -v --deep './$2'/Contents/qt-plugins/imageformats/*.dylib ;
    codesign -s 'Developer ID Application: $1' -f -v --deep './$2' ;
    codesign --verify --verbose=4 './$2' ;
    spctl -a -t exec -vv './$2';
    tar cvfj zulip-remotesigning_signed.tar.bz2 './$2' ;
    scp zulip-remotesigning_signed.tar.bz2 $VM_USERNAME@$VM_IP:/tmp/ ;"

mv "$2" "$2_unsigned.app"
tar xvfj /tmp/zulip-remotesigning_signed.tar.bz2
