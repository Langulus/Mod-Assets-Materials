///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Raymarch.hpp"
#include "Scene.hpp"

using namespace Nodes;


/// Raymarch function                                                         
///   @param {0} - scene function symbol                                      
///   @param {1} - precision                                                  
///   @param {2} - max raymarching distance                                   
///   @param {3} - far stride, used by the hybrid marcher                     
///                if the distance from the root is high enough we use d      
///                instead of log(d)                                          
///   @param {4} - base stride, used by the hybrid marcher                    
///                determines how fast the root finder moves in, needs to be  
///                lowered when dealing with thin "slices". the potential     
///                problem is the intersector crossing the function twice in  
///                one step                                                   
///   @param {5} - a minimum step size                                        
///   @param {6} - max number of raymarching steps                            
constexpr Token RaymarchFunction = R"shader(
   struct RaymarchResult {{
      float mDepth;
   }};

   // This modified(!) implementation of Log-Bisect-Raymarching is
   // based on nimitz's original code, without his permission or endorsement:
   // https://www.shadertoy.com/view/4sSXzD
   // License: https://creativecommons.org/licenses/by-nc-sa/3.0/
   float Bisect(in CameraResult camera, in float near, in float far) {{
      float mid = 0.0;
      float sgn = sign({0}(camera.mDirection * near + camera.mOrigin));
      for (int i = 0; i < 6; i++) {{
         mid = (near + far) * 0.5;
         float d = {0}(camera.mDirection * mid + camera.mOrigin);
         if (abs(d) < {1})
            break;

         d * sgn < 0.0 ? far = mid : near = mid;
      }}

      return (near + far) * 0.5;
   }}

   RaymarchResult Raymarch(in CameraResult camera) {{
      float t = 0.0;
      float d = {0}(camera.mDirection * t + camera.mOrigin);
      float sgn = sign(d);
      float told = t;

      for (int i = 0; i <= {6}; i++) {{
         if (abs(d) < {1} || t >= {2})
            break;
         else if (i == {6})
            t = {2};

         if (sign(d) != sgn) {{
            t = Bisect(camera, told, t);
            break;
         }}

         told = t;
         if (d > 1.0)
            t += d * {3};
         else
            t += log(abs(d) + 1.0 + {5}) * {4};

         d = {0}(camera.mDirection * t + camera.mOrigin);
      }}

      return RaymarchResult(1.0 - t / {2});
   }}
)shader";


/// Raymarcher node descriptor-constructor                                    
///   @param desc - raymarch descriptor                                       
Raymarch::Raymarch(const Descriptor& desc)
   : Node {MetaOf<Raymarch>(), desc} {
   // Extract RaymarchConfig.mDetail                                    
   mDescriptor.ExtractTrait<Traits::Count>(mDetail);
   LANGULUS_ASSERT(mDetail > 0, Material,
      "Bad raymarch iterations", mDetail);

   // Extract RaymarchConfig.mPrecision                                 
   //         RaymarchConfig.mFarMax                                    
   //         RaymarchConfig.mFarStride                                 
   //         RaymarchConfig.mBaseStride                                
   //         RaymarchConfig.mMinStep                                   
   mDescriptor.ExtractTrait<Traits::Argument>(
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
Symbol Raymarch::Generate() {
   // Generate children first                                           
   Descend();

   // In order to raymarch, we require child scene nodes                
   Symbols scenes;
   ForEachChild([&scenes](Nodes::Scene& scene) {
      scenes << scene.GenerateSDF();
   });

   LANGULUS_ASSERT(!scenes.IsEmpty(), Material,
      "No scenes available for raymarcher");

   if (scenes.GetCount() > 1)
      TODO(); // another SDFUnion indirection required here

   // Add raymarching functions and dependencies                        
   AddDefine("Raymarch", TemplateFill(RaymarchFunction,
      scenes[0].mCode, mPrecision, mFarMax, mFarStride, mBaseStride, 
      mMinStep, mDetail)
   );

   return Expose<Raymarch>("Raymarch({})", MetaOf<Camera>());
}
