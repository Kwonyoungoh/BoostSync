add_executable (BoostSync "main.cpp" "BoostSync.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET BoostSync PROPERTY CXX_STANDARD 20)
endif()

target_link_libraries(BoostSync PRIVATE 
	Boost::system 
	Boost::thread 
	hiredis::hiredis 
	nlohmann_json::nlohmann_json 
)

if(WIN32)
	target_link_libraries(BoostSync PRIVATE redis++::redis++)
else()
	target_link_libraries(BoostSync PRIVATE redis++::redis++_static)
endif()