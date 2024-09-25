///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
///                                                                           
#include "Value.hpp"
#include "../Material.hpp"

using namespace Nodes;


/// Value node constructor                                                    
///   @param parent - the node's parent                                       
Value::Value(Node* parent)
   : Resolvable {this}
   , Node {parent, {}} {}

/// Generate code for the value - turn any verbs executed on it to their GLSL 
/// code equivalent, and add animation functions if such are needed.          
const Symbol& Value::Generate() {
   Descend();
   TODO();
   return NoSymbol;
}
