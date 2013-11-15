#The MIT License (MIT)

#Copyright (c) 2013 Nicholas Hutchinson <nshutchinson@gmail.com>

#Permission is hereby granted, free of charge, to any person obtaining a copy
#of this software and associated documentation files (the "Software"), to deal
#in the Software without restriction, including without limitation the rights
#to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#copies of the Software, and to permit persons to whom the Software is
#furnished to do so, subject to the following conditions:

#The above copyright notice and this permission notice shall be included in
#all copies or substantial portions of the Software.

#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
#THE SOFTWARE.

include (CMakeParseArguments)
include (CTAddCompilerFlags)

function (CTObjCARCSetEnabled value scope)
  cmake_parse_arguments(args "DIRECTORY" "" "TARGETS;SOURCES" ${ARGV})
  __SanitiseBoolean(value)

  if (scope STREQUAL "DIRECTORY")
    if (CMAKE_GENERATOR STREQUAL "Xcode" AND
        CMAKE_CURRENT_SOURCE_DIR STREQUAL "${CMAKE_SOURCE_DIR}")
      set (CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC ${value} PARENT_SCOPE)
    elseif (value)
      add_definitions(-fobjc-arc)
    else ()
      add_definitions(-fno-objc-arc)
    endif ()

  elseif (scope STREQUAL "SOURCES")
    if (value)
      CTAddCompilerFlags(SOURCES ${args_SOURCES} FLAGS -fobjc-arc)
    else ()
      CTAddCompilerFlags(SOURCES ${args_SOURCES} FLAGS -fno-objc-arc)
    endif ()

  elseif (scope STREQUAL "TARGETS")
    if (CMAKE_GENERATOR STREQUAL "Xcode")
      set_property(TARGET ${args_TARGETS}
                   PROPERTY XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC ${value})
    elseif (value)
      CTAddCompilerFlags(TARGETS ${args_TARGETS} FLAGS -fobjc-arc)
    else ()
      CTAddCompilerFlags(TARGETS ${args_TARGETS} FLAGS -fno-objc-arc)
    endif ()
  else ()
    message(FATAL_ERROR "Invalid scope: ${scope}")
  endif ()
endfunction ()

# 0, NO, FALSE  -> NO
# ...           -> YES
function (__SanitiseBoolean variable)
  if (${${variable}})
    set (${variable} YES PARENT_SCOPE)
  else ()
    set (${variable} NO PARENT_SCOPE)
  endif ()
endfunction ()
