///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Light.hpp"

using namespace Nodes;


/// Light node creation                                                       
///   @param desc - descriptor for the node                                   
Light::Light(const Descriptor& desc)
   : Node {MetaOf<Light>(), desc} { }

/// Generate light definition code                                            
void Light::GenerateDefinition() {
   //TODO();
}

/// Generate light usage code with all modifiers                              
void Light::GenerateUsage() {
   //TODO();
}

/// Generate the shader stages                                                
void Light::Generate() {
   VERBOSE_NODE("Generating code...");
   Descend();
   Consume();

   GenerateDefinition();
   GenerateUsage();
}
