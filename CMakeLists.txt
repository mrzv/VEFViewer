cmake_minimum_required  (VERSION 3.1)
project                 (VEFViewer)

# Default to Release
if                      (NOT CMAKE_BUILD_TYPE)
    set                 (CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
    set_property        (CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif                   (NOT CMAKE_BUILD_TYPE)

set                     (CMAKE_CXX_STANDARD 11)

set                     (NANOGUI_DIR            ext/nanogui)
set                     (NANOGUI_BUILD_EXAMPLE  OFF CACHE BOOL " " FORCE)
set                     (NANOGUI_BUILD_PYTHON   OFF CACHE BOOL " " FORCE)
set                     (NANOGUI_BUILD_SHARED   OFF CACHE BOOL " " FORCE)
set                     (NANOGUI_INSTALL        OFF CACHE BOOL " " FORCE)
add_subdirectory        (${NANOGUI_DIR})

set                     (NANOGUI_LIBRARY nanogui)
include_directories     (${CMAKE_SOURCE_DIR}/include
                         ${NANOGUI_DIR}/include
                         SYSTEM ${NANOGUI_DIR}/ext/eigen
                         SYSTEM ${NANOGUI_DIR}/ext/glfw/include
                         ${NANOGUI_DIR}/ext/nanovg/src)

if                      (APPLE)
  set                   (CMAKE_CXX_FLAGS "-stdlib=libc++")
  # Find standard libraries needed for OpenGL/GLFW
  find_library          (cocoa_library Cocoa)
  find_library          (opengl_library OpenGL)
  find_library          (corevideo_library CoreVideo)
  find_library          (iokit_library IOKit)
  set                   (extra_libs ${cocoa_library} ${opengl_library} ${corevideo_library} ${iokit_library})
  set                   (extra_source src/darwin.mm)
elseif                  ("${CMAKE_SYSTEM}" MATCHES "Linux")
  set                   (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set                   (extra_libs GL Xxf86vm Xrandr Xinerama Xcursor Xi X11 pthread dl)
endif                   ()

# ZLib
find_package            (ZLIB REQUIRED)
include_directories     (${ZLIB_INCLUDE_DIRS})

# HDF5
find_package            (HDF5 REQUIRED)
include_directories     (${HDF5_INCLUDE_DIRS})

# Shaders
function                (bin2c shader_name)
    add_custom_command  (OUTPUT             ${CMAKE_BINARY_DIR}/${shader_name}.c
                         COMMAND            xxd -i ${shader_name} ${CMAKE_BINARY_DIR}/${shader_name}.c
                         WORKING_DIRECTORY  ${CMAKE_SOURCE_DIR}/shaders
                         MAIN_DEPENDENCY    ${CMAKE_SOURCE_DIR}/shaders/${shader_name})
endfunction             (bin2c)

set                     (shader_sources
                         vertex.vrt     vertex.frg
                         edge.vrt       edge.frg
                         sphere.vrt
                         triangle.vrt   triangle.frg)

foreach                 (shader_source ${shader_sources})
    bin2c               (${shader_source})
    set                 (shaders ${shaders} ${shader_source}.c)
endforeach              ()

add_library             (format         src/format.cc src/posix.cc)
add_executable          (VEFViewer      VEFViewer.cpp  ${shaders} src/parse_stl.cpp)
target_link_libraries   (VEFViewer      format ${NANOGUI_LIBRARY} ${extra_libs} ${ZLIB_LIBRARIES} ${HDF5_C_LIBRARIES})
