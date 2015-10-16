project( zulip )
cmake_minimum_required( VERSION 2.8.6 )
set( CMAKE_MODULE_PATH  "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules" )

set( ZULIP_VERSION_MAJOR 0 )
set( ZULIP_VERSION_MINOR 5 )
set( ZULIP_VERSION_PATCH 1 )

set( ZULIP_VERSION ${ZULIP_VERSION_MAJOR}.${ZULIP_VERSION_MINOR}.${ZULIP_VERSION_PATCH} )
set( ZULIP_VERSION_SHORT "${ZULIP_VERSION}" )

set( ZULIP_APPLICATION_NAME "Zulip")

# enforce proper symbol exporting on all platforms
add_definitions( "-fvisibility=hidden" )
# enforce using constBegin, constEnd for const-iterators
add_definitions( "-DQT_STRICT_ITERATORS" )

# make predefined install dirs available everywhere
include( GNUInstallDirs )

# Allow building with Qt5
option(BUILD_WITH_QT5 "Try to find Qt5 and build with it" OFF)

if (APPLE)
    set(QT_USE_QTWEBKIT FALSE)
    set(QT_COMPONENTS QtCore QtGui QtNetwork QtSvg QtXml)
else()
    set(QT_USE_QTWEBKIT TRUE)
    set(QT_COMPONENTS QtCore QtGui QtNetwork QtSvg QtXml QtWebkit)
endif()

# Generates windows installer with NSIS
INCLUDE( ZulipCPack.cmake )

# Dependencies

if(BUILD_WITH_QT5)
    find_package(Qt5Core QUIET)
    if( Qt5Core_DIR )
        find_package(Qt5Widgets QUIET)
        find_package(Qt5Network QUIET)
        message(STATUS "Building against Qt5")

        macro(qt_wrap_cpp)
            qt5_wrap_cpp(${ARGN})
        endmacro()
        macro(qt_wrap_ui)
            qt5_wrap_ui(${ARGN})
        endmacro()
        macro(qt_add_resources)
            qt5_add_resources(${ARGN})
        endmacro()

        include_directories(${Qt5Core_INCLUDES})
        include_directories(${Qt5Widgets_INCLUDES})
        add_definitions(${Qt5Widgets_DEFINITIONS})

        set(QT5_BUILD ON)
    endif()
else()
    find_program(QT_QMAKE_EXECUTABLE NAMES qmake4 qmake-qt4 qmake qmake-mac)
    find_package( Qt4 COMPONENTS ${QT_COMPONENTS} REQUIRED )

    macro(qt_wrap_cpp)
        qt4_wrap_cpp(${ARGN})
    endmacro()
    macro(qt_wrap_ui)
        qt4_wrap_ui(${ARGN})
    endmacro()
    macro(qt_add_resources)
        qt4_add_resources(${ARGN})
    endmacro()

    include( ${QT_USE_FILE} )
    add_definitions( ${QT_DEFINITIONS} )

    set(QT4_BUILD ON)
endif()

if(QT4_BUILD AND NOT APPLE AND NOT WIN32)
    find_package( Phonon 4.6.0 REQUIRED )
endif()

# Configure Qt
set( QT_USE_QTNETWORK TRUE )

# Output directories
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )

# Flag controlling whether this is a local server + SSO build or not
set( SSO_BUILD OFF CACHE BOOL "Whether or not to make the desktop app use the SSO login page")
if( SSO_BUILD )
  message("Building SSO version of Zulip Desktop")
endif()

# make uninstall support
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)

add_custom_target(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

add_subdirectory( thirdparty )
add_subdirectory( src )

if (APPLE)
    add_subdirectory(src/mac/launchhelper)
endif()
