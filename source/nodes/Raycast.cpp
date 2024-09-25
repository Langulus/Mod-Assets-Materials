///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
///                                                                           
#include "Raycast.hpp"

using namespace Nodes;


/// Raycaster node descriptor-constructor                                     
///   @param desc - raycast descriptor                                        
Raycast::Raycast(Describe&& descriptor)
   : Resolvable {this}
   , Node {*descriptor} { }

/// Generate the raycaster code                                               
///   @return the raycaster function template                                 
const Symbol& Raycast::Generate() {
   Descend();
   TODO();
   return NoSymbol;
}
