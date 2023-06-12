cmake_minimum_required(VERSION 3.24)
project(LangulusModAssetsMaterials)

# Build the module                                                          
add_library(LangulusModAssetsMaterials SHARED
    source/MaterialLibrary.cpp
    source/Material.cpp
    source/Node.cpp
    source/GLSL.cpp
    source/nodes/Camera.cpp
    source/nodes/FBM.cpp
    source/nodes/Light.cpp
    source/nodes/Raster.cpp
    source/nodes/Raycast.cpp
    source/nodes/Raymarch.cpp
    source/nodes/Raytrace.cpp
    source/nodes/Root.cpp
    source/nodes/Scene.cpp
    source/nodes/SceneSDF.cpp
    source/nodes/SceneTriangles.cpp
    source/nodes/SceneLines.cpp
    source/nodes/Texture.cpp
    source/nodes/Transform.cpp
    source/nodes/Value.cpp
)

target_link_libraries(LangulusModAssetsMaterials 
    PRIVATE     Langulus
)

if(LANGULUS_ENABLE_TESTING)
    enable_testing()
	add_subdirectory(test)
endif()