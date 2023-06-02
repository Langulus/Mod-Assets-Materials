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
      float mPrecision;
      float mFarMax;
      float mFarStride;
      float mBaseStride;
      float mMinStep;
      int mDetail;
      SceneSDF mScene;

   public:
      Raymarch(const Descriptor&);

      void Generate() final;

   private:
      void GenerateDefinition();
   };

} // namespace Nodes