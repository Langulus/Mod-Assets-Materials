cmake_minimum_required(VERSION 3.24)
project(LangulusModAssetsMaterials
    VERSION         1.0.0
    DESCRIPTION     "Langulus plug-in module for generating and managing materials and shaders"
    HOMEPAGE_URL    https://langulus.com
)

file(GLOB_RECURSE
	LANGULUS_MOD_ASSETS_MATERIALS_SOURCES 
	LIST_DIRECTORIES FALSE CONFIGURE_DEPENDS
	source/*.cpp
)

# Build the module                                                              
add_library(LangulusModAssetsMaterials SHARED ${LANGULUS_MOD_ASSETS_MATERIALS_SOURCES})

target_link_libraries(LangulusModAssetsMaterials 
    PRIVATE     Langulus
)

if(LANGULUS_TESTING)
    enable_testing()
	add_subdirectory(test)
endif()