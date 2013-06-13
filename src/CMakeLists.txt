project( humbug )

set( humbug_SRCS
      cookiejar.cpp
      HumbugTrayIcon.cpp
      HumbugWebBridge.cpp
      HumbugWindow.cpp
      HumbugAboutDialog.cpp
      WheelFilter.cpp
      main.cpp )

set( humbug_HDRS
      cookiejar.h
      HumbugTrayIcon.h
      HumbugWebBridge.h
      HumbugAboutDialog.h
      WheelFilter.h
      HumbugWindow.h
      webview/HWebView.h )

set( humbug_UIS
    HumbugWindow.ui
    HumbugAboutDialog.ui )

set( humbug_RESOURCES resources.qrc )

if (APPLE)
    set(EXTRA_SRCS webview/HWebView_mac.mm)

    set(EXTRA_LIBS /System/Library/Frameworks/Foundation.framework
                   /System/Library/Frameworks/WebKit.framework
                   /System/Library/Frameworks/CoreGraphics.framework)

    qt4_wrap_cpp(EXTRA_MOCS webview/HWebView.h)
else()
    set(EXTRA_SRCS webview/HWebView.cpp)
endif()

include_directories(
    .
    ${QT_INCLUDES}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PHONON_INCLUDES}
)

qt4_add_resources( humbug_RESOURCES_RCC ${humbug_RESOURCES})
qt4_wrap_ui( humbug_UI_H ${humbug_UIS} )

add_executable( humbug WIN32 MACOSX_BUNDLE ${humbug_SRCS} ${EXTRA_SRCS} ${EXTRA_MOCS} ${humbug_RESOURCES_RCC} ${humbug_UI_H} )
set_target_properties( humbug
    PROPERTIES
        AUTOMOC TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_BINARY_DIR}/Info.plist"
)

target_link_libraries( humbug
    ${PHONON_LIBS}
    ${QT_LIBRARIES}
    ${EXTRA_LIBS}
)

# Platform specifics
if (APPLE)
# Enable sparkle when we have it
#  option(ENABLE_SPARKLE "Sparkle updating" ON)
#  find_library(SPARKLE Sparkle)
#  if (ENABLE_SPARKLE AND SPARKLE)
#    set(HAVE_SPARKLE ON)
#    set( OS_SPECIFIC_LINK_LIBRARIES ${OS_SPECIFIC_LINK_LIBRARIES} ${SPARKLE} )
#  endif(ENABLE_SPARKLE AND SPARKLE)
#  FILE(COPY ${CMAKE_SOURCE_DIR}/admin/mac/sparkle_pub.pem
#    DESTINATION "${CMAKE_BINARY_DIR}/humbug.app/Contents/Resources")

  FILE(READ ${CMAKE_SOURCE_DIR}/admin/mac/Info.plist plist)
  STRING( REPLACE "HUMBUG_VERSION"
              ${HUMBUG_VERSION}
              edited_plist # save in this variable
              "${plist}" # from the contents of this var
          )
  FILE( WRITE ${CMAKE_BINARY_DIR}/Info.plist "${edited_plist}" )

endif (APPLE)
