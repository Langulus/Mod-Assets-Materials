///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
///                                                                           
#include "MaterialLibrary.hpp"
#include "nodes/Root.hpp"
#include "nodes/Camera.hpp"
#include "nodes/FBM.hpp"
#include "nodes/Light.hpp"
#include "nodes/Raster.hpp"
#include "nodes/Raymarch.hpp"
#include "nodes/Raytrace.hpp"
#include "nodes/Scene.hpp"
#include "nodes/Texture.hpp"
#include "nodes/Value.hpp"

#include <Langulus/Math/Normal.hpp>
#include <Langulus/Math/Primitives.hpp>
#include <Langulus/Math/Angle.hpp>
#include <Langulus/Math/SimplexNoise.hpp>
#include <Langulus/Math/Config.hpp>

LANGULUS_DEFINE_MODULE(
   MaterialLibrary, 9, "AssetsMaterials",
   "Module for reading, writing, and generating GLSL/HLSL shaders for visualizing materials", "",
   MaterialLibrary, Material, GLSL,
   Nodes::Camera,
   Nodes::FBM,
   Nodes::Light,
   Nodes::Raster,
   Nodes::Raymarch,
   Nodes::Raytrace,
   Nodes::Root,
   Nodes::Scene,
   Nodes::Texture,
   Nodes::Value
)


/// Module construction                                                       
///   @param runtime - the runtime that owns the module                       
///   @param descriptor - instructions for configuring the module             
MaterialLibrary::MaterialLibrary(Runtime* runtime, const Many&)
   : Resolvable {this}
   , Module     {runtime} {
   Logger::Verbose(Self(), "Initializing...");
   Math::RegisterNormals();
   Math::RegisterPrimitives();
   Math::RegisterAngles();
   Math::RegisterTraits();
   Math::RegisterVerbs();
   Logger::Verbose(Self(), "Initialized");
}

/// First stage destruction                                                   
void MaterialLibrary::Teardown() {
   mMaterials.Teardown();
}

/// Create/Destroy materials                                                  
///   @param verb - the creation/destruction verb                             
void MaterialLibrary::Create(Verb& verb) {
   mMaterials.Create(this, verb);
}