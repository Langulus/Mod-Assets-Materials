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
   ///   Raytracing material node                                             
   struct Raytrace final : Node {
      using CTTI_Abstract  = No;
      using CTTI_Bases     = Node;

      Raytrace(Describe);
      Symbol const& Generate();
   };
}