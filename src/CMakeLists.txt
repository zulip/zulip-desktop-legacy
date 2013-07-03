project( humbug )

include (AddAppIconMacro)

# Build type
IF( NOT CMAKE_BUILD_TYPE STREQUAL "Release" )
    MESSAGE( "Building in debug mode, enabling domain selector" )
    SET( DEBUG_BUILD ON )
ENDIF()

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
    set(EXTRA_SRCS webview/HWebView_mac.mm mac/HumbugApplication_mac.mm mac/PlatformInterface.mm)

    set(EXTRA_LIBS /System/Library/Frameworks/Foundation.framework
                   /System/Library/Frameworks/AppKit.framework
                   /System/Library/Frameworks/WebKit.framework
                   /System/Library/Frameworks/ApplicationServices.framework
                   /System/Library/Frameworks/Carbon.framework)

    qt4_wrap_cpp(MAC_EXTRA_MOCS webview/HWebView.h PlatformInterface.h)
    set(EXTRA_SRCS ${MAC_EXTRA_MOCS} ${EXTRA_SRCS})
else()
    set(EXTRA_SRCS webview/HWebView.cpp)
endif()

if (WIN32)
  set( CMAKE_BUILD_TYPE "Release" )

  add_definitions( /DNOMINMAX )
  add_definitions( /DWIN32_LEAN_AND_MEAN )
  add_definitions( -static-libgcc )
  add_definitions( -DUNICODE )

  include (CheckCXXSourceCompiles)

  check_cxx_source_compiles( "
      #include <shobjidl.h>
      int main() {
	  THUMBBUTTON foo;
	  return 0;
      }
      "
      HAVE_THUMBBUTTON )

  set(EXTRA_SRCS ${EXTRA_SRCS} win/PlatformInterface_win.cpp)

  set( EXTRA_LIBS
      ${EXTRA_LIBS}
      qtsparkle
      "secur32.dll"
      "crypt32.dll"
      "iphlpapi.a"
      "ws2_32.dll"
      "dnsapi.dll"
      "dsound.dll"
      "winmm.dll"
      "advapi32.dll"
  )

  kde4_add_app_icon( humbug_SRCS "${CMAKE_SOURCE_DIR}/admin/icon/humbug-icon-*.png" )

  qt4_wrap_cpp(WIN_EXTRA_MOCS PlatformInterface.h)
  set(EXTRA_SRCS ${WIN_EXTRA_MOCS} ${EXTRA_SRCS})
endif()

if (NOT WIN32 AND NOT APPLE)
    set(EXTRA_SRCS ${EXTRA_SRCS} PlatformInterface.cpp)
endif()

include_directories(
    .
    ${CMAKE_SOURCE_DIR}/thirdparty/qtsparkle/src
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

  option(ENABLE_GROWL "Growl Notifications" ON)
  find_library(GROWL Growl)
  if (ENABLE_GROWL AND GROWL)
    set(HAVE_GROWL ON)
    set(EXTRA_LIBS ${EXTRA_LIBS} ${GROWL})
    message("Found Growl.framework, enabling notifications")
  endif()

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
  FILE(COPY ${CMAKE_SOURCE_DIR}/src/audio/humbug.wav
         DESTINATION "${CMAKE_BINARY_DIR}/humbug.app/Contents/Resources")
  FILE(COPY "${CMAKE_SOURCE_DIR}/admin/mac/Growl Registration Ticket.growlRegDict"
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
