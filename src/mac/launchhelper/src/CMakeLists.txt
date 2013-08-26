project (zulip-launchhelper)

set(launchhelper_SRCS main.cpp ZulipAppHelperDelegate.mm)

add_executable(ZulipAppHelper MACOSX_BUNDLE ${launchhelper_SRCS})

set_target_properties(ZulipAppHelper
    PROPERTIES
        AUTOMOC TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist"
)

target_link_libraries(ZulipAppHelper /System/Library/Frameworks/Foundation.framework
                                     /System/Library/Frameworks/AppKit.framework)
