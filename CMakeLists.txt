cmake_minimum_required  (VERSION 3.0.2)
project                 (VEFViewer)

find_path               (NANOGUI_INCLUDE_DIR    nanogui/nanogui.h)
find_library            (NANOGUI_LIBRARY        nanogui)
find_path               (EIGEN_INCLUDE_DIR      eigen3/Eigen/Core)
find_path               (NANOVG_INCLUDE_DIR     nanovg.h)
include_directories     (${CMAKE_SOURCE_DIR}/include
                         ${NANOGUI_INCLUDE_DIR}
                         SYSTEM ${EIGEN_INCLUDE_DIR}/eigen3
                         ${NANOVG_INCLUDE_DIR})


set                     (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
if                      (APPLE)
  set                   (CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++")
  # Find standard libraries needed for OpenGL/GLFW
  find_library          (cocoa_library Cocoa)
  find_library          (opengl_library OpenGL)
  find_library          (corevideo_library CoreVideo)
  find_library          (iokit_library IOKit)
  set                   (extra_libs ${cocoa_library} ${opengl_library} ${corevideo_library} ${iokit_library})
  set                   (extra_source src/darwin.mm)

  find_library          (GLFW3_LIBRARY          glfw3)
elseif                  ("${CMAKE_SYSTEM}" MATCHES "Linux")
  set                   (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  set                   (extra_libs GL Xxf86vm Xrandr Xinerama Xcursor Xi X11 pthread dl)

  find_library          (GLFW3_LIBRARY          glfw)
endif                   ()

add_executable          (VEFViewer      VEFViewer.cpp)
target_link_libraries   (VEFViewer      ${NANOGUI_LIBRARY} ${GLFW3_LIBRARY} ${extra_libs})
