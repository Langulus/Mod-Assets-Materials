///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
///                                                                           
#include "Raymarch.hpp"
#include "Scene.hpp"
#include "Camera.hpp"

using namespace Nodes;


/// Raymarcher node descriptor-constructor                                    
///   @param desc - raymarch descriptor                                       
Raymarch::Raymarch(Describe&& descriptor)
   : Resolvable {this}
   , Node {*descriptor} {
   // Extract RaymarchConfig.mDetail                                    
   mDescriptor.ExtractTrait<Traits::Count>(mDetail);
   LANGULUS_ASSERT(mDetail > 0, Material,
      "Bad raymarch iterations", mDetail);

   // Extract RaymarchConfig.mPrecision                                 
   //         RaymarchConfig.mFarMax                                    
   //         RaymarchConfig.mFarStride                                 
   //         RaymarchConfig.mBaseStride                                
   //         RaymarchConfig.mMinStep                                   
   mDescriptor.ExtractTrait<Traits::Input>(
      mPrecision, mFarMax, mFarStride, mBaseStride, mMinStep
   );
   LANGULUS_ASSERT(mPrecision > 0, Material,
      "Bad raymarching precision", mPrecision);
   LANGULUS_ASSERT(mFarMax > 0, Material,
      "Bad raymarching far plane", mFarMax);
   LANGULUS_ASSERT(mFarStride > 0, Material,
      "Bad raymarching far stride", mFarStride);
   LANGULUS_ASSERT(mBaseStride > 0, Material,
      "Bad raymarching base stride", mBaseStride);
   LANGULUS_ASSERT(mMinStep > 0, Material,
      "Bad raymarching min step", mMinStep);
}

/// Generate raymarcher code                                                  
///   @return the output symbol                                               
const Symbol& Raymarch::Generate() {
   // Generate children first                                           
   Descend();

   // In order to raymarch, we require child scene nodes                
   Symbols scenes;
   ForEachChild([&scenes](Nodes::Scene& scene) {
      scenes << scene.GenerateSDF();
   });

   LANGULUS_ASSERT(scenes, Material, "No scenes available for raymarcher");

   if (scenes.GetCount() > 1)
      TODO(); // another SDFUnion indirection required here

   // Add raymarching functions and dependencies                        
   AddDefine("Raymarch", Text::TemplateRt(RaymarchFunction,
      scenes[0].mCode, mPrecision, mFarMax, mFarStride, mBaseStride, 
      mMinStep, mDetail)
   );

   return ExposeData<Raymarch>("Raymarch({})", MetaOf<Camera>());
}
