///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Raymarch.hpp"

using namespace Nodes;


/// Raymarcher settings structure                                             
///   @param {0} - precision                                                  
///   @param {1} - max raymarching distance                                   
///   @param {2} - far stride, used by the hybrid marcher                     
///                if the distance from the root is high enough we use d      
///                instead of log(d)                                          
///   @param {3} - base stride, used by the hybrid marcher                    
///                determines how fast the root finder moves in, needs to be  
///                lowered when dealing with thin "slices". the potential     
///                problem is the intersector crossing the function twice in  
///                one step                                                   
///   @param {4} - a minimum step size                                        
///   @param {5} - max number of raymarching steps                            
constexpr Token RaymarchConfig = R"shader(
   struct Raymarcher {{
      float mPrecision;
      float mFarMax;
      float mFarStride;
      float mBaseStride;
      float mMinStep;
      int mDetail;
   }};
   const Raymarcher raymarchConfig = Raymarcher(
      {0}, {1}, {2}, {3}, {4}, {5}
   );
)shader";

/// SDF scene function                                                        
///   @param {0} - scene code                                                 
constexpr Token SceneFunction = R"shader(
   float Scene(in vec3 point) {{
      return {0};
   }}
)shader";

/// Raymarch function                                                         
constexpr Token RaymarchFunction = R"shader(
   struct RaymarchResult {{
      float mDepth;
   }};

   // This modified(!) implementation of Log-Bisect-Raymarching is
   // based on nimitz's original code, without his permission or endorsement:
   // https://www.shadertoy.com/view/4sSXzD
   // License: https://creativecommons.org/licenses/by-nc-sa/3.0/
   float Bisect(in RaymarchConfig config, in CameraResult camera, in float near, in float far) {{
      float mid = 0.0;
      float sgn = sign(Scene(camera.mDirection * near + camera.mOrigin));
      for (int i = 0; i < 6; i++) {{
         mid = (near + far) * 0.5;
         float d = SceneSDF(camera.mDirection * mid + camera.mOrigin);
         if (abs(d) < config.mPrecision)
            break;

         d * sgn < 0.0 ? far = mid : near = mid;
      }}

      return (near + far) * 0.5;
   }}

   RaymarchResult Raymarch(in RaymarchConfig config, in CameraResult camera) {{
      float t = 0.0;
      float d = Scene(camera.mDirection * t + camera.mOrigin);
      float sgn = sign(d);
      float told = t;
      bool doBisect = false;
      for (int i = 0; i <= config.mDetail; i++) {{
         if (abs(d) < config.mPrecision || t >= config.mFarMax)
            break;
         else if (i == config.mDetail)
            t = config.mFarMax;

         if (sign(d) != sgn) {{
            doBisect = true;
            break;
         }}

         told = t;
         if (d > 1.0)
            t += d * config.mFarStride;
         else
            t += log(abs(d) + 1.0 + config.mMinStep) * config.mBaseStride;

         d = Scene(camera.mDirection * t + camera.mOrigin);
      }}

      if (doBisect)
         t = Bisect(config, camera, told, t);

      return RaymarchResult(t);
   }}
)shader";

/// Raymarch usage                                                            
constexpr Token RaymarchUsage = R"shader(
   RaymarchResult raymarchResult = Raymarch(raymarchConfig, camResult);
   if (raymarchResult.mDepth >= raymarchConfig.mFarMax)
      discard;
   raymarchResult.mDepth = 1.0 - raymarchResult.mDepth / raymarchConfig.m;
)shader";


/// Raymarcher node descriptor-constructor                                    
///   @param desc - raymarch descriptor                                       
Raymarch::Raymarch(const Descriptor& desc)
   : Node {MetaOf<Raymarch>(), desc}
   , mScene {this, desc} {
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
      "Bad RaymarchConfig.mPrecision", mPrecision);
   LANGULUS_ASSERT(mFarMax > 0, Material,
      "Bad RaymarchConfig.mFarMax", mFarMax);
   LANGULUS_ASSERT(mFarStride > 0, Material,
      "Bad RaymarchConfig.mFarStride", mFarStride);
   LANGULUS_ASSERT(mBaseStride > 0, Material,
      "Bad RaymarchConfig.mBaseStride", mBaseStride);
   LANGULUS_ASSERT(mMinStep > 0, Material,
      "Bad RaymarchConfig.mMinStep", mMinStep);

   // Register the outputs                                              
   Expose<Traits::Color, Real>("rmrResult.mDepth");
}

/// Generate raymarcher definition code                                       
void Raymarch::GenerateDefinition() {
   // In order to raymarch, we require an SDF scene                     
   GLSL functions, scene;
   mScene.GenerateCode(functions, scene);
   LANGULUS_ASSERT(!scene.IsEmpty(), Material,
      "No scene available for raymarcher");

   // Add raymarching functions and dependencies                        
   functions += TemplateFill(SceneFunction, scene);
   functions += TemplateFill(RaymarchConfig, 
      mPrecision, mFarMax, mFarStride, mBaseStride, mMinStep, mDetail);
   functions += RaymarchFunction;

   // Commit                                                            
   Commit(ShaderToken::Functions, functions);
   Commit(ShaderToken::Transform, RaymarchUsage);
}

/// Generate the shader stages                                                
void Raymarch::Generate() {
   VERBOSE_NODE("Generating code...");
   Descend();
   Consume();
   GenerateDefinition();
}
