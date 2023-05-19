///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "../MContent.hpp"

/// Light node creation																			
///	@param parent - the parent node														
///	@param verb - light creator verb														
MaterialNodeLight::MaterialNodeLight(MaterialNode* parent, const Verb& verb)
   : MaterialNode{ MetaData::Of<MaterialNodeLight>(), parent, verb } { }

/// Illumination/darkening based on dot products										
///	@param verb - projection verb															
void MaterialNodeLight::Illuminate(Verb& verb) {
   verb << this;
}

/// Generate light definition code															
void MaterialNodeLight::GenerateDefinition() {
   //TODO();
}

/// Generate light usage code with all modifiers										
void MaterialNodeLight::GenerateUsage() {
   //TODO();
}

/// Generate the shader stages																
void MaterialNodeLight::Generate() {
   PC_VERBOSE_MATERIAL("Generating code...");
   Descend();
   Consume();

   GenerateDefinition();
   GenerateUsage();
}
