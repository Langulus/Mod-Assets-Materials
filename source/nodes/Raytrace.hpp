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
   ///   Raytracing material node                                             
   ///                                                                        
   struct Raytrace final : Node {
      LANGULUS(ABSTRACT) false;
      LANGULUS_BASES(Node);

      Raytrace(Describe&&);
      const Symbol& Generate();
   };

} // namespace Nodes