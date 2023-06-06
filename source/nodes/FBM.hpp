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
   ///   Fractal Brownian Motion node                                         
   ///                                                                        
   struct FBM : Node {
   private:
      // Code used to generate octaves                                  
      Code mCode;
      // Base weight value                                              
      Real mBaseWeight {0.5};
      // Number of octaves                                              
      Count mOctaveCount {2};

   public:
      FBM(const Descriptor&);

      Symbol Generate() final;

      NOD() operator Debug() const;
   };

} // namespace Nodes