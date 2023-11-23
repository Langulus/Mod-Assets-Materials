add_executable(LangulusModAssetsMaterialsTest
	Main.cpp
	TestMaterial.cpp
)

target_link_libraries(LangulusModAssetsMaterialsTest
	PRIVATE		Langulus
				Catch2
)

add_test(
	NAME		LangulusModAssetsMaterialsTest
	COMMAND		LangulusModAssetsMaterialsTest
	WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)