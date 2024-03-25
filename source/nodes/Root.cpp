///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Root.hpp"
#include "../Material.hpp"

using namespace Nodes;


/// Root node creation                                                        
///   @param producer - the producer material                                 
///   @param desc - the node descriptor                                       
Root::Root(Material* producer, const Neat& desc)
   : Resolvable {MetaOf<Root>()}
   , Node {producer, desc} {
   // Satisfy the rest of the descriptor                                
   // This is just a root node, so we can safely create anything in it  
   producer->Couple(desc); //TODO crappy solution
   InnerCreate();
}

/// Generate the shader stages                                                
const Symbol& Root::Generate() {
   // Just generate children                                            
   Descend();
   TODO();
   return NoSymbol;
}
