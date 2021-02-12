cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
project(mc-autojoin)

if(POLICY CMP0063)
  cmake_policy(SET CMP0063 NEW)
endif()

add_compile_options(
	-Wall
	-Wextra
	-Wshadow
	-Wno-unused-parameter
	-Wmissing-noreturn
	-Wmissing-prototypes
	-Wstrict-prototypes
)

find_package(PkgConfig REQUIRED)
pkg_search_module(NL REQUIRED libnl-3.0)
pkg_search_module(NL_ROUTE REQUIRED libnl-route-3.0)

add_executable(mc-autojoin autojoin.c)
target_link_libraries(mc-autojoin ${NL_LIBRARIES} ${NL_ROUTE_LIBRARIES})
target_include_directories(mc-autojoin PRIVATE ${NL_INCLUDE_DIRS} ${NL_ROUTE_LIBRARIES})

install(TARGETS mc-autojoin RUNTIME DESTINATION bin)
