project( zulip )
cmake_minimum_required( VERSION 2.8.6 )
set( CMAKE_MODULE_PATH  "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules" )

set( ZULIP_VERSION_MAJOR 0 )
set( ZULIP_VERSION_MINOR 3 )
set( ZULIP_VERSION_PATCH 5 )

set( ZULIP_VERSION ${ZULIP_VERSION_MAJOR}.${ZULIP_VERSION_MINOR}.${ZULIP_VERSION_PATCH} )
set( ZULIP_VERSION_SHORT "${ZULIP_VERSION}" )

set( ZULIP_APPLICATION_NAME "Zulip")

# enforce proper symbol exporting on all platforms
add_definitions( "-fvisibility=hidden" )
# enforce using constBegin, constEnd for const-iterators
add_definitions( "-DQT_STRICT_ITERATORS" )

# make predefined install dirs available everywhere
include( GNUInstallDirs )

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
find_package( Qt4 COMPONENTS ${QT_COMPONENTS} REQUIRED )

if(NOT APPLE AND NOT WIN32)
    find_package( Phonon 4.6.0 REQUIRED )
endif()

# Configure Qt
set( QT_USE_QTNETWORK TRUE )
include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )

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
