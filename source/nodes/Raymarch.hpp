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
   ///   Raymarch material node                                               
   ///                                                                        
   struct Raymarch : Node {
   private:
      // Raymarcher precision                                           
      float mPrecision {0.008f};
      // Max raymarching distance                                       
      float mFarMax {1000.0f};
      // Far stride, used by the hybrid marcher                         
      // If the distance from the root is high enough we use d          
      // instead of log(d)                                              
      float mFarStride {0.3f};
      // Base stride, used by the hybrid marcher                        
      // Determines how fast the root finder moves in, needs to be      
      // lowered when dealing with thin "slices". The potential         
      // problem is the intersector crossing the function twice in      
      // one step                                                       
      float mBaseStride {0.75f};
      // Minimum step size                                              
      float mMinStep {0.1f};
      // Max number of raymarching steps                                
      int mDetail {60};

   public:
      Raymarch(const Descriptor&);

      Symbol Generate() final;
   };

} // namespace Nodes