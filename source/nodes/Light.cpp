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
Light::Light(Describe&& descriptor)
   : Node {MetaOf<Light>(), *descriptor} { }

/// Generate the light functionality                                          
///   @return the light function template                                     
const Symbol& Light::Generate() {
   // Generate children first, if any                                   
   Descend();
   TODO();
   return NoSymbol;
}
