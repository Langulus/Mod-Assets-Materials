project(LangulusModAssetsMaterialsTest)

add_executable(LangulusModAssetsMaterialsTest
	Main.cpp
	TestGLSL.cpp
)

target_link_libraries(LangulusModAssetsMaterialsTest
	PRIVATE		Langulus
				Catch2
)

add_test(
	NAME		LangulusModAssetsMaterialsTest
	COMMAND		LangulusModAssetsMaterialsTest
)

langulus_copy_dlls(LangulusModAssetsMaterialsTest ON LangulusModAssetsMaterialsTest)