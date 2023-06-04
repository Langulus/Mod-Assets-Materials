///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "../Node.hpp"
#include "SceneSDF.hpp"

namespace Nodes
{

   ///                                                                        
   ///   Raymarch material node                                               
   ///                                                                        
   struct Raymarch : Node {
   private:
      float mPrecision {0.008f};
      float mFarMax {1000.0f};
      float mFarStride {0.3f};
      float mBaseStride {0.75f};
      float mMinStep {0.1f};
      int mDetail {60};
      SceneSDF mScene;

   public:
      Raymarch(const Descriptor&);

      void Generate() final;

   private:
      void GenerateDefinition();
   };

} // namespace Nodes