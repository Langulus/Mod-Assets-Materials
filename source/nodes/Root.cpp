///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
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
   : Node {MetaOf<Root>(), producer, desc} {
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
