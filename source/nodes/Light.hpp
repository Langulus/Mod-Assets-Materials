///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "../Node.hpp"


namespace Nodes
{

   ///                                                                        
   ///   Light material node                                                  
   ///                                                                        
   struct Light final : Node {
      LANGULUS(ABSTRACT) false;
      LANGULUS_BASES(Node);

      Light(Describe&&);
      const Symbol& Generate();
   };

} // namespace Nodes