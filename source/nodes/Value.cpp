///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
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
   : Node {MetaOf<Value>(), parent, {}} {}

/// Generate code for the value - turn any verbs executed on it to their GLSL 
/// code equivalent, and add animation functions if such are needed.          
const Symbol& Value::Generate() {
   Descend();
   TODO();
}
