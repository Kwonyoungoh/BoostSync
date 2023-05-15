cmake_minimum_required (VERSION 3.8)

project ("BoostSync")


# Find Boost
find_package(Boost REQUIRED)
if(Boost_FOUND)
  message(STATUS "Boost found!")
  include_directories(${Boost_INCLUDE_DIRS})
endif()

# Find hiredis
find_path(HIREDIS_INCLUDE_DIR hiredis/hiredis.h HINTS /usr/include)
find_library(HIREDIS_LIBRARY NAMES hiredis HINTS /usr/lib /usr/lib/x86_64-linux-gnu)
if(HIREDIS_INCLUDE_DIR AND HIREDIS_LIBRARY)
  message(STATUS "hiredis found!")
  include_directories(${HIREDIS_INCLUDE_DIR})
endif()

# Find nlohmann_json
find_package(nlohmann_json 3.2.0 REQUIRED)
if(nlohmann_json_FOUND)
  message(STATUS "nlohmann_json found!")
endif()

# 하위 프로젝트를 포함합니다.
add_subdirectory ("BoostSync")
