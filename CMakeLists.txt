cmake_minimum_required(VERSION 3.5)
project(galtape CXX)
option(STATIC_BUILD "Create static build" OFF)

set(CMAKE_CXX_STANDARD 14)

if (STATIC_BUILD)
    if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      set(CMAKE_EXE_LINKER_FLAGS "-Bstatic")
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        if(NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
          set(CMAKE_EXE_LINKER_FLAGS "-static")
        endif()
    else()
        set(CMAKE_EXE_LINKER_FLAGS "-static")
    endif()
endif()

include_directories(3rdparty/cxxopts/include 3rdparty/wave/src)

aux_source_directory(src/ SOURCE_FILES)

add_library(wave
  3rdparty/wave/src/wave/header/data_header.h
  3rdparty/wave/src/wave/header/data_header.cc
  3rdparty/wave/src/wave/header/riff_header.h
  3rdparty/wave/src/wave/header/riff_header.cc
  3rdparty/wave/src/wave/header/fmt_header.h
  3rdparty/wave/src/wave/header/fmt_header.cc
  3rdparty/wave/src/wave/header/wave_header.h
  3rdparty/wave/src/wave/header/wave_header.cc

  3rdparty/wave/src/wave/header.h
  3rdparty/wave/src/wave/header.cc
  3rdparty/wave/src/wave/header_list.h
  3rdparty/wave/src/wave/header_list.cc

  3rdparty/wave/src/wave/error.h
  3rdparty/wave/src/wave/file.h
  3rdparty/wave/src/wave/file.cc
)

add_executable(galtape ${SOURCE_FILES} ${INCLUDE_FILES})
target_link_libraries(galtape PRIVATE wave)