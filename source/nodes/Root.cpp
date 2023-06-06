///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Root.hpp"

using namespace Nodes;


/// Root node creation                                                        
///   @param producer - the producer material                                 
///   @param desc - the node descriptor                                       
Root::Root(Material* producer, const Descriptor& desc)
   : Node {MetaOf<Root>(), producer, desc} { }

/// Generate the shader stages                                                
Symbol Root::Generate() {
   // Just generate children                                            
   Descend();
   return {};
}
