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
   ///   Root node                                                            
   ///                                                                        
   struct Root final : Node {
      using CTTI_Abstract = No;
      LANGULUS_BASES(Node);

      Root(Material*, Many const&);

      auto Generate() -> const Symbol&;
   };

} // namespace Nodes