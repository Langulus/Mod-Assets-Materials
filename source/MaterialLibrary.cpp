///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
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

#include <Math/Normal.hpp>
#include <Math/Primitives.hpp>
#include <Math/Angle.hpp>
#include <Math/SimplexNoise.hpp>
#include <Math/Config.hpp>

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
MaterialLibrary::MaterialLibrary(Runtime* runtime, const Neat&)
   : Resolvable {this}
   , Module {runtime}
   , mMaterials {this} {
   Logger::Verbose(Self(), "Initializing...");
   Math::RegisterNormals();
   Math::RegisterPrimitives();
   Math::RegisterAngles();
   Math::RegisterTraits();
   Math::RegisterVerbs();
   Logger::Verbose(Self(), "Initialized");
}

/// Create/Destroy materials                                                  
///   @param verb - the creation/destruction verb                             
void MaterialLibrary::Create(Verb& verb) {
   mMaterials.Create(verb);
}