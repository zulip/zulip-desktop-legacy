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

# Annoyingly, compiler flags in CMake are a space-separated string,
# not a list. This function allows for more consistent behaviour.
function (CTAddCompilerFlags scope)
  cmake_parse_arguments(args "" "" "FLAGS" ${ARGN})

  set (space_separated_flags "")
  foreach (flag IN LISTS args_FLAGS)
    set (space_separated_flags "${space_separated_flags} ${flag}")
  endforeach ()

  if (scope STREQUAL "TARGETS")
    set_property(TARGET ${args_UNPARSED_ARGUMENTS} APPEND_STRING
      PROPERTY COMPILE_FLAGS "${space_separated_flags}")

  elseif (scope STREQUAL "SOURCES")
    set_property(SOURCE ${args_UNPARSED_ARGUMENTS} APPEND_STRING
      PROPERTY COMPILE_FLAGS "${space_separated_flags}")

  elseif (scope STREQUAL "DIRECTORY")
    # Using add_definitions() feels slightly seedy--these are *flags*, not
    # *definitions* dammit!--but the docs say you can do it. CMake has no
    # COMPILE_FLAGS directory property, and we can't really use
    # CMAKE_<LANG>_FLAGS because they're passed to the linker as well, which
    # is surprising.
    add_definitions(${args_FLAGS})
  else ()
    message(FATAL_ERROR "Invalid scope '${scope}' passed CTAddCompilerFlags")
  endif ()
endfunction ()
