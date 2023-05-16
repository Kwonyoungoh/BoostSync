add_executable (BoostSync "main.cpp" "BoostSync.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET BoostSync PROPERTY CXX_STANDARD 20)
endif()
target_link_libraries(BoostSync ${Boost_LIBRARIES} ${HIREDIS_LIBRARY} nlohmann_json::nlohmann_json cpp_redis ${REDIS_PLUS_PLUS_LIB})
target_include_directories(BoostSync PRIVATE ${Boost_INCLUDE_DIRS} ${HIREDIS_INCLUDE_DIR} ${REDIS_PLUS_PLUS_HEADER})