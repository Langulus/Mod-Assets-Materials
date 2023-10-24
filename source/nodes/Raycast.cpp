///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Raycast.hpp"

using namespace Nodes;


/// Raycaster node descriptor-constructor                                     
///   @param desc - raycast descriptor                                        
Raycast::Raycast(const Neat& desc)
   : Node {MetaOf<Raycast>(), desc} { }

/// Generate the raycaster code                                               
///   @return the raycaster function template                                 
const Symbol& Raycast::Generate() {
   Descend();
   TODO();
   return NoSymbol;
}
