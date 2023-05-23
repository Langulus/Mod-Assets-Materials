///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "../MContent.hpp"


/// Raycaster node creation                                                   
///   @param parent - the parent node                                          
///   @param verb - the raycast creator verb                                    
MaterialNodeRaycast::MaterialNodeRaycast(MaterialNode* parent, const Verb& verb)
   : MaterialNode{ MetaData::Of<MaterialNodeRaycast>(), parent, verb } { }

/// Generate raycaster definition code                                          
void MaterialNodeRaycast::GenerateDefinition() {
   TODO();
}

/// Generate the shader stages                                                
void MaterialNodeRaycast::Generate() {
   PC_VERBOSE_MATERIAL("Generating code...");
   Descend();
   Consume();

   GenerateDefinition();
}
