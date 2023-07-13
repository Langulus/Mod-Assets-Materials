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
   ///   Value node                                                           
   ///                                                                        
   /// Just a node that allows modification of inputs/outputs and execution   
   /// of verbs in a temporary context. Also supports keyframes via the verb  
   /// interface - you can execute verbs with time/frequency charge           
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