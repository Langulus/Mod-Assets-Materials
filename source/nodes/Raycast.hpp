///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "../Node.hpp"


namespace Nodes
{

   ///                                                                        
   ///   Raycast material node                                                
   ///                                                                        
   struct Raycast final : Node {
      LANGULUS(ABSTRACT) false;
      LANGULUS_BASES(Node);

      Raycast(const Descriptor&);
      const Symbol& Generate();
   };

} // namespace Nodes