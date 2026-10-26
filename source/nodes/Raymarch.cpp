///                                                                           
/// Langulus::Module::Art::Materials                                          
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
///   @param desc raymarch descriptor                                         
Raymarch::Raymarch(Describe&& descriptor)
   : Downcastable {this}
   , Node         {*descriptor} {
   // Extract RaymarchConfig.mDetail                                    
   mDescriptor.ExtractTag<Tags::Count>(mDetail);
   LglsAssert(mDetail > 0, "Bad raymarch iterations", mDetail);

   // Extract RaymarchConfig.mPrecision                                 
   //         RaymarchConfig.mFarMax                                    
   //         RaymarchConfig.mFarStride                                 
   //         RaymarchConfig.mBaseStride                                
   //         RaymarchConfig.mMinStep                                   
   mDescriptor.ExtractTag<Tags::Input>(
      mPrecision, mFarMax, mFarStride, mBaseStride, mMinStep
   );
   LglsAssert(mPrecision  > 0, "Bad raymarching precision", mPrecision);
   LglsAssert(mFarMax     > 0, "Bad raymarching far plane", mFarMax);
   LglsAssert(mFarStride  > 0, "Bad raymarching far stride", mFarStride);
   LglsAssert(mBaseStride > 0, "Bad raymarching base stride", mBaseStride);
   LglsAssert(mMinStep    > 0, "Bad raymarching min step", mMinStep);
}

/// Generate raymarcher code                                                  
///   @return the output symbol                                               
Symbol const& Raymarch::Generate() {
   // Generate children first                                           
   Descend();

   // In order to raymarch, we require child scene nodes                
   Symbols scenes;
   ForEachChild([&scenes](Nodes::Scene& scene) {
      scenes << scene.GenerateSDF();
   });

   LglsAssert(scenes, "No scenes available for raymarcher");

   if (scenes.GetCount() > 1)
      TODO(); // another SDFUnion indirection required here

   // Add raymarching functions and dependencies                        
   AddDefine("Raymarch", Text::Format(RaymarchFunction,
      scenes[0].mCode, mPrecision, mFarMax, mFarStride, mBaseStride, 
      mMinStep, mDetail)
   );

   return ExposeData<Raymarch>("Raymarch({})", MetaOf<Camera>());
}
