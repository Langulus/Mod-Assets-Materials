///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Value.hpp"
#include "../Material.hpp"

using namespace Nodes;


/// Value node constructor                                                    
///   @param parent - the node's parent                                       
Value::Value(Node* parent)
   : Resolvable {MetaOf<Value>()}
   , Node {parent, {}} {}

/// Generate code for the value - turn any verbs executed on it to their GLSL 
/// code equivalent, and add animation functions if such are needed.          
const Symbol& Value::Generate() {
   Descend();
   TODO();
   return NoSymbol;
}
