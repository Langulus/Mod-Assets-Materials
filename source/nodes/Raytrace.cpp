///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "../MContent.hpp"


/// Raytracer node creation																	
///	@param parent - the parent node														
///	@param verb - the raymarcher creator verb											
MaterialNodeRaytrace::MaterialNodeRaytrace(MaterialNode* parent, const Verb& verb)
   : MaterialNode{ MetaData::Of<MaterialNodeRaytrace>(), parent, verb } { }

/// Generate raytracer definition code														
void MaterialNodeRaytrace::GenerateDefinition() {
   TODO();
}

/// Generate the shader stages																
void MaterialNodeRaytrace::Generate() {
   PC_VERBOSE_MATERIAL("Generating code...");
   Descend();
   Consume();

   GenerateDefinition();
}
