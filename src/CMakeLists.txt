project( humbug )

set( humbug_SRCS
      HumbugTrayIcon.cpp
      HumbugWebBridge.cpp
      HumbugWindow.cpp
      main.cpp )

set( humbug_HDRS
      HumbugTrayIcon.h
      HumbugWebBridge.h
      HumbugWindow.h )

set( humbug_UIS
    HumbugWindow.ui )

set( humbug_RESOURCES resources.qrc )

include_directories(
    .
    ${QT_INCLUDES}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PHONON_INCLUDES}
)

qt4_add_resources( humbug_RESOURCES_RCC ${humbug_RESOURCES})
qt4_wrap_ui( humbug_UI_H ${humbug_UIS} )

add_executable( humbug WIN32 MACOSX_BUNDLE ${humbug_SRCS} ${humbug_RESOURCES_RCC} ${humbug_UI_H} )
set_target_properties( humbug
    PROPERTIES
        AUTOMOC TRUE
       # uncomment once we have a plist for osx MACOSX_BUNDLE_INFO_PLIST "${CMAKE_BINARY_DIR}/Info.plist"
)

target_link_libraries( humbug
    ${PHONON_LIBS}
    ${QT_LIBRARIES}
)
