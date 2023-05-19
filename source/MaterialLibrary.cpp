///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "MaterialLibrary.hpp"

LANGULUS_DEFINE_MODULE(
   MaterialLibrary, 9, "AssetsMaterials",
   "Module for reading, writing, and generating GLSL/HLSL shaders for visualizing materials", "",
   MaterialLibrary, Material
)


/// Module construction                                                       
///   @param runtime - the runtime that owns the module                       
///   @param descriptor - instructions for configuring the module             
MaterialLibrary::MaterialLibrary(Runtime* runtime, const Descriptor&)
   : A::AssetModule {MetaOf<MaterialLibrary>(), runtime}
   , mMaterials {this} {
   Logger::Verbose(Self(), "Initializing...");
   Logger::Verbose(Self(), "Initialized");
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