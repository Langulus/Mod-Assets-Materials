///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
///                                                                           
#include "Root.hpp"
#include "../Material.hpp"

using namespace Nodes;


/// Root node creation                                                        
///   @param producer - the producer material                                 
///   @param desc - the node descriptor                                       
Root::Root(Material* producer, const Many& desc)
   : Resolvable {this}
   , Node       {producer, desc} {
   // Satisfy the rest of the descriptor                                
   // This is just a root node, so we can safely create anything in it  
   producer->Couple(desc); //TODO crappy solution
   InnerCreate();
}

/// Generate the shader stages                                                
auto Root::Generate() -> const Symbol& {
   // Just generate children                                            
   Descend();
   TODO();
   return NoSymbol;
}
