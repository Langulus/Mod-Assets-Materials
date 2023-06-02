///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Raytrace.hpp"

using namespace Nodes;


/// Raytracer node descriptor-constructor                                     
///   @param desc - raytrace descriptor                                       
Raytrace::Raytrace(const Descriptor& desc)
   : Node {MetaOf<Raytrace>(), desc} { }

/// Generate raytracer definition code                                        
void Raytrace::GenerateDefinition() {
   TODO();
}

/// Generate the shader stages                                                
void Raytrace::Generate() {
   VERBOSE_NODE("Generating code...");
   Descend();
   Consume();
   GenerateDefinition();
}
