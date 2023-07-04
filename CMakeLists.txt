cmake_minimum_required (VERSION 3.8)

project ("BoostSync")

	find_package(Boost REQUIRED COMPONENTS system thread)
	find_package(hiredis REQUIRED)
	find_package(redis++ REQUIRED)
	find_package(nlohmann_json 3.2.0 REQUIRED)

add_subdirectory ("BoostSync")
