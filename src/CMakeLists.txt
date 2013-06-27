project( humbug )

include (AddAppIconMacro)

set( humbug_SRCS
      cookiejar.cpp
      HumbugWebBridge.cpp
      HumbugWindow.cpp
      HumbugAboutDialog.cpp
      IconRenderer.cpp
      main.cpp )

set( humbug_UIS
    HumbugWindow.ui
    HumbugAboutDialog.ui )

set( humbug_RESOURCES resources.qrc )


qt4_wrap_cpp(EXTRA_MOCS HumbugApplication.h)

if (APPLE)
    set(EXTRA_SRCS webview/HWebView_mac.mm mac/HumbugApplication_mac.mm mac/Setup.mm)

    set(EXTRA_LIBS /System/Library/Frameworks/Foundation.framework
                   /System/Library/Frameworks/AppKit.framework
                   /System/Library/Frameworks/WebKit.framework
                   /System/Library/Frameworks/ApplicationServices.framework
                   /System/Library/Frameworks/Carbon.framework)

    qt4_wrap_cpp(MAC_EXTRA_MOCS webview/HWebView.h)
    set(EXTRA_SRCS ${MAC_EXTRA_MOCS} ${EXTRA_SRCS})
else()
    set(EXTRA_SRCS webview/HWebView.cpp)
endif()

if (WIN32)
  SET( CMAKE_BUILD_TYPE "Release" )

  ADD_DEFINITIONS( /DNOMINMAX )
  ADD_DEFINITIONS( /DWIN32_LEAN_AND_MEAN )
  ADD_DEFINITIONS( -static-libgcc )
  ADD_DEFINITIONS( -DUNICODE )

  check_cxx_source_compiles( "
      #include <shobjidl.h>
      int main() {
	  THUMBBUTTON foo;
	  return 0;
      }
      "
      HAVE_THUMBBUTTON )

  SET( EXTRA_LIBS
      ${EXTRA_LIBS}
      "secur32.dll"
      "crypt32.dll"
      "iphlpapi.a"
      "ws2_32.dll"
      "dnsapi.dll"
      "dsound.dll"
      "winmm.dll"
      "advapi32.dll"
  )
endif()

include_directories(
    .
    ${QT_INCLUDES}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PHONON_INCLUDES}
)

qt4_add_resources( humbug_RESOURCES_RCC ${humbug_RESOURCES})
qt4_wrap_ui( humbug_UI_H ${humbug_UIS} )

kde4_add_app_icon( humbug_SRCS "${CMAKE_SOURCE_DIR}/admin/icon/humbug-icon-*.png" )

add_executable( humbug WIN32 MACOSX_BUNDLE ${humbug_SRCS} ${EXTRA_SRCS} ${EXTRA_MOCS} ${humbug_RESOURCES_RCC} ${humbug_UI_H} )
set_target_properties( humbug
    PROPERTIES
        AUTOMOC TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_BINARY_DIR}/Info.plist"
)

install(TARGETS humbug DESTINATION bin)

# Platform specifics
if (APPLE)
# Enable sparkle when we have it
  option(ENABLE_SPARKLE "Sparkle updating" ON)
  find_library(SPARKLE Sparkle)
  if (ENABLE_SPARKLE AND SPARKLE)
    set(HAVE_SPARKLE ON)
    set(EXTRA_LIBS ${EXTRA_LIBS} ${SPARKLE})
  endif(ENABLE_SPARKLE AND SPARKLE)
  FILE(COPY ${CMAKE_SOURCE_DIR}/admin/mac/sparkle_pub.pem
    DESTINATION "${CMAKE_BINARY_DIR}/humbug.app/Contents/Resources")

  # Uses Darwin kernel version.
  # 9.8.0  -> 10.5/Leopard
  # 10.4.0 -> 10.6/Snow Leopard
  # 11.x.x -> Lion
  # 12.x.x -> Mountain Lion
  string(REGEX MATCH "[0-9]+" DARWIN_VERSION ${CMAKE_HOST_SYSTEM_VERSION})
  if (DARWIN_VERSION GREATER 11)
    SET(MOUNTAIN_LION 1)
  elseif (DARWIN_VERSION GREATER 10)
    SET(LION 1)
  elseif (DARWIN_VERSION GREATER 9)
    SET(SNOW_LEOPARD 1)
  elseif (DARWIN_VERSION GREATER 8)
    SET(LEOPARD 1)
  endif (DARWIN_VERSION GREATER 11)

  FILE(READ ${CMAKE_SOURCE_DIR}/admin/mac/Info.plist plist)
  STRING( REPLACE "HUMBUG_VERSION"
              ${HUMBUG_VERSION}
              edited_plist # save in this variable
              "${plist}" # from the contents of this var
          )
  FILE( WRITE ${CMAKE_BINARY_DIR}/Info.plist "${edited_plist}" )

  FILE(COPY ${CMAKE_SOURCE_DIR}/src/images/Humbug.icns
         DESTINATION "${CMAKE_BINARY_DIR}/humbug.app/Contents/Resources")

endif (APPLE)

target_link_libraries( humbug
    ${PHONON_LIBS}
    ${QT_LIBRARIES}
    ${EXTRA_LIBS}
)

# Config.h file for cmake flags conversion into #-defines
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/Config.h)
