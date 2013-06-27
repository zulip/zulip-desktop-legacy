#!/bin/bash

if [ "$1" = "-c" ] ; then
echo "Continuing last download.."
rm -rvf vlc/
else
echo "Update archive..."
fi

rm -rvf vlc/


echo "Downloading vlc and plugins archive..."
wget -c http://download.tomahawk-player.org/test/pvlc.tar.bz2

echo "Extract binary..."
tar xvjf pvlc.tar.bz2

# this is for vlc-2.x
rm -rvf \
    video_*/ \
    gui/ \
    **/libold* \
    **/libvcd* \
    **/libdvd* \
    **/liblibass* \
    **/libx264* \
    **/libschroe* \
    **/liblibmpeg2* \
    **/libstream_out_* \
    **/libmjpeg_plugin* \
    **/libh264_plugin* \
    **/libzvbi_plugin* \
    **/lib*sub* \
    services_discovery/ \
    visualization/ \
    control/ \
    misc/ \
    **/libi420* \
    **/libi422* \
    mux/ \
    stream_filter/ \
    **/libtheora_plugin* \
    **/liblibbluray_plugin* \
    **/libdtv_plugin* \
    **/*.dll.a \
    **/*.la


echo "Downloaded and stripped VLC"

