project( zulip )

include (AddAppIconMacro)

set( zulip_SRCS
      cookiejar.cpp
      ZulipWebBridge.cpp
      ZulipWindow.cpp
      ZulipAboutDialog.cpp
      IconRenderer.cpp
      Logger.cpp
      ZulipApplication.cpp
      main.cpp )

set( zulip_UIS
    ZulipWindow.ui
    ZulipAboutDialog.ui )

set( zulip_RESOURCES resources.qrc )

if (APPLE)
    set(EXTRA_SRCS ../thirdparty/qocoa/qbutton_mac.mm
                   webview/HWebView_mac.mm
                   mac/ZulipApplication_mac.mm
                   mac/PlatformInterface.mm
                   mac/NSData+Base64.mm
                   mac/NSArray+Blocks.mm)

    set(EXTRA_LIBS /System/Library/Frameworks/Foundation.framework
                   /System/Library/Frameworks/AppKit.framework
                   /System/Library/Frameworks/WebKit.framework
                   /System/Library/Frameworks/ApplicationServices.framework
                   /System/Library/Frameworks/Carbon.framework
                   /System/Library/Frameworks/ServiceManagement.framework)

    qt4_wrap_cpp(MAC_EXTRA_MOCS webview/HWebView.h PlatformInterface.h)
    set(EXTRA_SRCS ${MAC_EXTRA_MOCS} ${EXTRA_SRCS})
else()
    set(EXTRA_SRCS ../thirdparty/qocoa/qbutton_nonmac.cpp webview/HWebView.cpp)
endif()

qt4_wrap_cpp(EXTRA_MOCS ../thirdparty/qocoa/qbutton.h Utils.h)

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

  kde4_add_app_icon( zulip_SRCS "${CMAKE_SOURCE_DIR}/admin/icon/zulip-icon-*.png" )

  qt4_wrap_cpp(WIN_EXTRA_MOCS PlatformInterface.h)
  set(EXTRA_SRCS ${WIN_EXTRA_MOCS} ${EXTRA_SRCS})
endif()

if (NOT WIN32 AND NOT APPLE)
    set(EXTRA_SRCS ${EXTRA_SRCS} PlatformInterface.cpp)

    include_directories(${PHONON_INCLUDES} ${CMAKE_SOURCE_DIR}/..)
    set(EXTRA_LIBS ${EXTRA_LIBS} ${PHONON_LIBS})
endif()

include_directories(
    .
    ${CMAKE_SOURCE_DIR}/thirdparty/qtsparkle/src
    ${QT_INCLUDES}
    ${QJSON_INCLUDE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

qt4_add_resources( zulip_RESOURCES_RCC ${zulip_RESOURCES})
qt4_wrap_ui( zulip_UI_H ${zulip_UIS} )

add_executable( zulip WIN32 MACOSX_BUNDLE ${zulip_SRCS} ${EXTRA_SRCS} ${EXTRA_MOCS} ${zulip_RESOURCES_RCC} ${zulip_UI_H} )
set_target_properties( zulip
    PROPERTIES
        AUTOMOC TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_BINARY_DIR}/Info.plist"
)

install(TARGETS zulip DESTINATION bin)

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
    DESTINATION "${CMAKE_BINARY_DIR}/zulip.app/Contents/Resources")

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

 if (SSO_BUILD)
    SET(SPARKLE_URL "https://zulip.com/dist/apps/sso/mac/sparkle.xml")
  else()
    SET(SPARKLE_URL "https://zulip.com/dist/apps/mac/sparkle.xml")
  endif()

  FILE(READ ${CMAKE_SOURCE_DIR}/admin/mac/Info.plist plist)
  STRING( REPLACE "ZULIP_VERSION"
              ${ZULIP_VERSION}
              edited_plist # save in this variable
              "${plist}" # from the contents of this var
          )
  STRING( REPLACE "SPARKLE_URL"
              ${SPARKLE_URL}
              edited_plist # save in this variable
              "${edited_plist}" # from the contents of this var
          )
  FILE( WRITE ${CMAKE_BINARY_DIR}/Info.plist "${edited_plist}" )

  FILE(COPY ${CMAKE_SOURCE_DIR}/src/images/Zulip.icns
         DESTINATION "${CMAKE_BINARY_DIR}/zulip.app/Contents/Resources")
  FILE(COPY ${CMAKE_SOURCE_DIR}/src/audio/zulip.wav
         DESTINATION "${CMAKE_BINARY_DIR}/zulip.app/Contents/Resources")
  FILE(COPY "${CMAKE_SOURCE_DIR}/admin/mac/Growl Registration Ticket.growlRegDict"
         DESTINATION "${CMAKE_BINARY_DIR}/zulip.app/Contents/Resources")
endif ()

if (WIN32)
    SET(WINDOWS 1)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    SET(LINUX 1)
endif (WIN32)

if (LINUX)
  FILE(READ ${CMAKE_SOURCE_DIR}/admin/linux/Zulip.desktop desktopFile)
  STRING(REPLACE "ZULIP_VERSION"
              ${ZULIP_VERSION}
              replacedDesktopFile # save in this variable
              "${desktopFile}" # from the contents of this var
          )
  FILE( WRITE ${CMAKE_BINARY_DIR}/Zulip.desktop "${replacedDesktopFile}" )
  INSTALL( FILES ${CMAKE_BINARY_DIR}/Zulip.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications )

  FILE(GLOB _icons "${CMAKE_SOURCE_DIR}/admin/icon/zulip-icon-*.png")
  FOREACH( _file ${_icons} )
    STRING( REPLACE "${CMAKE_SOURCE_DIR}/admin/icon/zulip-icon-" "" _res ${_file} )
    STRING( REPLACE ".png" "" _res ${_res} )
    INSTALL( FILES ${_file} RENAME zulip.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${_res}/apps )
  ENDFOREACH( _file )

  INSTALL( FILES ${CMAKE_SOURCE_DIR}/admin/icon/zulip-desktop_32x32.xpm DESTINATION ${CMAKE_INSTALL_PREFIX}/share/pixmaps )
 endif()

target_link_libraries( zulip
    ${QT_LIBRARIES}
    ${QJSON_LIBRARIES}
    ${EXTRA_LIBS}
)

# Config.h file for cmake flags conversion into #-defines
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/Config.h)
