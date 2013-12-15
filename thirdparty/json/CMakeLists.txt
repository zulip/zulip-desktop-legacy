project(json-backported)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(JSON_SRCS qjson.cpp
    qjsondocument.cpp
    qjsonobject.cpp
    qjsonarray.cpp
    qjsonvalue.cpp
    qjsonwriter.cpp
    qjsonparser.cpp)

add_library(json SHARED ${JSON_SRCS})

target_link_libraries(json ${QT_LIBRARIES})

install(TARGETS json DESTINATION lib)