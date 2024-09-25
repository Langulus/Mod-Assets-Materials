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
   ///   Value node                                                           
   ///                                                                        
   ///   Just a node that allows modification of inputs/outputs and execution 
   /// of verbs in a temporary context. Also supports keyframes via the verb  
   /// interface - you can execute verbs with time/frequency charge           
   ///                                                                        
   struct Value final : Node {
      LANGULUS(ABSTRACT) false;
      LANGULUS_BASES(Node);

   private:
      Temporal mKeyframes;

   public:
      Value(Node*);

      const Symbol& Generate();
   };

} // namespace Nodes