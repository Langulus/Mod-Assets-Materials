file(GLOB_RECURSE
	LANGULUS_MOD_ASSETS_MATERIALS_TEST_SOURCES 
	LIST_DIRECTORIES FALSE CONFIGURE_DEPENDS
	*.cpp
)

add_langulus_test(LangulusModAssetsMaterialsTest
	SOURCES			${LANGULUS_MOD_ASSETS_MATERIALS_TEST_SOURCES}
	LIBRARIES		Langulus
	DEPENDENCIES    LangulusModAssetsMaterials
					LangulusModAssetsImages
					LangulusModFileSystem
)

# Make the write and read data dir for PhysFS, because it doesn't have access   
add_custom_command(
    TARGET LangulusModAssetsMaterialsTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
		"$<TARGET_FILE_DIR:LangulusModAssetsMaterialsTest>/data/assets/images"
    COMMAND ${CMAKE_COMMAND} -E copy
		"${CMAKE_CURRENT_SOURCE_DIR}/pebbles.png"
		"$<TARGET_FILE_DIR:LangulusModAssetsMaterialsTest>/data/assets/images/pebbles.png"
    COMMAND ${CMAKE_COMMAND} -E copy
		"${CMAKE_CURRENT_SOURCE_DIR}/border.png"
		"$<TARGET_FILE_DIR:LangulusModAssetsMaterialsTest>/data/assets/images/border.png"
)