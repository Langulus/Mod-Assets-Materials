///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
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

      Raycast(Describe&&);
      const Symbol& Generate();
   };

} // namespace Nodes