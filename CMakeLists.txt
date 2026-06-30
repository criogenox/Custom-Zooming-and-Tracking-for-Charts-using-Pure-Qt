cmake_minimum_required(VERSION 3.28)
set(PROJECT_NAME "trackplot")
project(${PROJECT_NAME} VERSION 1.0.0 LANGUAGES CXX)
set(TARGET ${PROJECT_NAME})
set(TARGET_LIB "${PROJECT_NAME}_lib")
set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})
#-----------#-----------#-----------#

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
#-----------#-----------#-----------#

set(MAIN_PATH "${PROJECT_SOURCE_DIR}/example")
set(SOURCE_PATH "${PROJECT_SOURCE_DIR}/src")
set(HEADERS_PATH "${PROJECT_SOURCE_DIR}/headers")
set(INCLUDE_PATH "${PROJECT_SOURCE_DIR}/include")
#-----------#-----------#-----------#

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Charts Concurrent)
#-----------#-----------#-----------#

# (1) Shared library: the reusable trackplot widget.
add_library(${TARGET_LIB} SHARED
        ${SOURCE_PATH}/customEvents.cpp
        ${INCLUDE_PATH}/customEvents.h)

target_compile_features(${TARGET_LIB} PUBLIC cxx_std_20)

# Public: consumers including customEvents.h need this on their include path.
target_include_directories(${TARGET_LIB} PUBLIC ${INCLUDE_PATH})

# Charts types appear in the public headers (QChartView, QXYSeries, ...), so it
# must be PUBLIC; the rest are implementation details and stay PRIVATE.
target_link_libraries(${TARGET_LIB}
        PUBLIC
        Qt6::Charts
        PRIVATE
        Qt6::Widgets
        Qt6::Gui
        Qt6::Concurrent)

install(TARGETS ${TARGET_LIB}
        LIBRARY DESTINATION libs)
##-----------#-----------#-----------#

# (2) Standalone example executable linked against the shared library.
add_executable(${TARGET}
        ${MAIN_PATH}/main.cpp
        ${MAIN_PATH}/testWindow.cpp
        ${HEADERS_PATH}/testWindow.h)

# Example-private headers (testWindow.h); customEvents.h comes transitively from
# the library's PUBLIC include directory.
target_include_directories(${TARGET} PRIVATE ${HEADERS_PATH})

target_link_libraries(${TARGET}
        PRIVATE
        ${TARGET_LIB}
        Qt6::Widgets
        Qt6::Gui)
##-----------#-----------#-----------#

# Profiling (-pg) is opt-in and scoped to the targets only, so it never leaks
# into Qt's generated moc/uic/rcc tooling builds.
if (CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_options(${TARGET_LIB} PRIVATE -pg)
    target_link_options(${TARGET_LIB} PRIVATE -pg)
    target_compile_options(${TARGET} PRIVATE -pg)
    target_link_options(${TARGET} PRIVATE -pg)
endif ()
##-----------#-----------#-----------#

