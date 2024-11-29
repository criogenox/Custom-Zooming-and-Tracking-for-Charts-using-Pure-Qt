cmake_minimum_required(VERSION 3.22)
set(PROJECT_NAME "chart_tracking_final")
project(${PROJECT_NAME} VERSION 1.0.0 LANGUAGES CXX)
set(TARGET ${PROJECT_NAME})
#-----------#-----------#-----------#

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#-----------#-----------#-----------#

set(MAIN_PATH "${PROJECT_SOURCE_DIR}/example")
set(SOURCE_PATH "${PROJECT_SOURCE_DIR}/src")
set(HEADERS_PATH "${PROJECT_SOURCE_DIR}/headers")
set(INCLUDE_PATH "${PROJECT_SOURCE_DIR}/include")
set(LIBS_PATH "${PROJECT_SOURCE_DIR}/libs")
#-----------#-----------#-----------#

include_directories(${HEADERS_PATH} ${INCLUDE_PATH})
#-----------#-----------#-----------#

file(GLOB_RECURSE CPPM "${MAIN_PATH}/*.cpp")
file(GLOB_RECURSE HS "${HEADERS_PATH}/*.h")
file(GLOB_RECURSE HI "${INCLUDE_PATH}/*.h")
file(GLOB_RECURSE CPPS "${SOURCE_PATH}/*.cpp")
set(PROJECT_SOURCES ${CPPM} ${CPPS} ${HS} ${HI})
#-----------#-----------#-----------#

find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Gui Widgets Charts)
#-----------#-----------#-----------#

add_executable(${TARGET} ${PROJECT_SOURCES})
target_link_libraries(${TARGET}
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Charts)
#-----------#-----------#-----------#
