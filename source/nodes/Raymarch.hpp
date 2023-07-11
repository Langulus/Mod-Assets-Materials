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
   struct Raymarch final : Node {
      LANGULUS(ABSTRACT) false;
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
      const Symbol& Generate();
   };

} // namespace Nodes


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
