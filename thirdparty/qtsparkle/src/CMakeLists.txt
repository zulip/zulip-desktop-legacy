cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(SOURCES
  appcast.cpp
  common.cpp
  compareversions.cpp
  followredirects.cpp
  uicontroller.cpp
  updatechecker.cpp
  updatedialog.cpp
  updater.cpp
)

set(MOC_SOURCES
  followredirects.h
  uicontroller.h
  updatechecker.h
  updatedialog.h
  updater.h
)

set(UI_SOURCES
  updatedialog.ui
)

qt4_wrap_cpp(MOC ${MOC_SOURCES})
qt4_wrap_ui(UIC ${UI_SOURCES})

add_library(qtsparkle
  ${SOURCES}
  ${MOC}
  ${UIC}
)

target_link_libraries(qtsparkle
  ${QT_LIBRARIES}
)