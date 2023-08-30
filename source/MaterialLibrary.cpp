///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
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
#include <Math/Primitives.hpp> //TODO move this to geometry mod
#include <Math/Angles.hpp> //TODO move this to geometry mod
#include <Math/Randomness/SimplexNoise.hpp>  //TODO move this to geometry mod
#include <Math/Verbs.hpp>  //TODO move this to geometry mod

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
   : A::AssetModule {MetaOf<MaterialLibrary>(), runtime}
   , mMaterials {this} {
   Logger::Verbose(Self(), "Initializing...");
   Logger::Verbose(Self(), "Initialized");

   //TODO move these to geometry mod
   (void)MetaOf<Math::Normal3>();
   (void)MetaOf<Traits::Bilateral>();
   Math::RegisterPrimitives();
   Math::RegisterAngles();
   Math::RegisterTraits();
   Math::RegisterVerbs();
}

/// Module update routine                                                     
///   @param dt - time from last update                                       
void MaterialLibrary::Update(Time) {

}

/// Create/Destroy materials                                                  
///   @param verb - the creation/destruction verb                             
void MaterialLibrary::Create(Verb& verb) {
   mMaterials.Create(verb);
}