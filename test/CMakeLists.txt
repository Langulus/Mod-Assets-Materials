project(LangulusModAssetsMaterialsTest)

add_executable(LangulusModAssetsMaterialsTest
	Main.cpp
	TestMod.cpp
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
langulus_copy_dlls_advanced(LangulusModAssetsMaterialsTest
	TO			LangulusModAssetsMaterialsTest
	FROM		LangulusModGLFW
				LangulusModVulkan
				LangulusModAssetsMaterials
)