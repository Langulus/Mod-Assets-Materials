///                                                                           
/// Langulus::Module::Art::Materials                                          
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
   ///   Light material node                                                  
   struct Light final : Node {
      using CTTI_Abstract  = No;
      using CTTI_Bases     = Node;

      Light(Describe&&);
      Symbol const& Generate();
   };
}