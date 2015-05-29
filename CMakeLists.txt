cmake_minimum_required(VERSION 2.8)
set(CMAKE_CONFIGURATION_TYPES Debug Release CACHE TYPE INTERNAL FORCE)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
project(sixfree)

add_definitions(-DBOOST_ALL_DYN_LINK)

find_package(Boost 1.55.0 COMPONENTS system filesystem thread log program_options REQUIRED)

file(GLOB FREEMEMORY_SOURCES *.cpp)

source_group(Source\ Files FILES ${FREEMEMORY_SOURCES})

file(GLOB FREEMEMORY_HEADERS *.hpp)

source_group(Source\ Files FILES ${FREEMEMORY_HEADERS})

#include_directories(${CMAKE_INCLUDE_PATH} src)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall -Werror -Wextra -std=c++11")

add_executable(
	${PROJECT_NAME}
	${FREEMEMORY_SOURCES}
	${FREEMEMORY_HEADERS}
)

target_link_libraries( ${PROJECT_NAME} ${Boost_LIBRARIES} pthread )
