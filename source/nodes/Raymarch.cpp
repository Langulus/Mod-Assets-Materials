///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "../MContent.hpp"


/// Raymarcher node creation                                                   
///   @param parent - the parent node                                          
///   @param verb - the raymarcher creator verb                                 
MaterialNodeRaymarch::MaterialNodeRaymarch(MaterialNode* parent, const Verb& verb)
   : MaterialNode{ MetaData::Of<MaterialNodeRaymarch>(), parent, verb } {
   // Verb argument can provide additional traits for the rasterizer      
   verb.GetArgument().ForEachDeep([&](const Trait&) {
      TODO();
   });
   Verb vcopy = verb;
   MaterialNode::Create(vcopy);

   // Verb argument can provide additional traits for the raymarcher      
   // iPrecision = 0.008 (tolerance)                                    
   // iDetail = 60 (max iterations)                                       
   // iFarMax = 1000.0   (max raymarch distance)                           
   // iFarStride = 0.3 (used by the hybrid marcher)                     
   //      - if the distance from the root is high enough we use d         
   //      - instead of log(d)                                             
   // iBaseStride = 0.75                                                
   //      - determines how fast the root finder moves in, needs to be      
   //      - lowered when dealing with thin "slices". the potential         
   //      - problem is the intersector crossing the function twice in      
   //      - one step                                                      
   // iMinStep = 0.1 (to cross faster, a minimum step size)               

   // Set default values if none were provided in verb                  
   if (!mSetup.IsDefined("iPrecision"))
      mSetup += "#define iPrecision 0.008\n";
   if (!mSetup.IsDefined("iDetail"))
      mSetup += "#define iDetail 60\n";
   if (!mSetup.IsDefined("iFarMax"))
      mSetup += "#define iFarMax 1000.0\n";
   if (!mSetup.IsDefined("iFarStride"))
      mSetup += "#define iFarStride 0.3\n";
   if (!mSetup.IsDefined("iBaseStride"))
      mSetup += "#define iBaseStride 0.75\n";
   if (!mSetup.IsDefined("iMinStep"))
      mSetup += "#define iMinStep 0.1\n";

   // Register the outputs                                                
   Expose<Traits::Color, real>("rmrResult.mDepth");
}

/// Generate raymarcher definition code                                       
void MaterialNodeRaymarch::GenerateDefinition() {
   // In order to raymarch, we require the SDF functions                  
   // for each child scene                                                
   GLSL functions, sdf;
   ForEachChild([&](MaterialNodeScene& scene) {
      scene.GenerateSDFCode(functions, sdf);
   });

   if (sdf.IsEmpty())
      throw Except::Content(pcLogSelfError << "No SDF available for raymarcher");

   // Generate SceneSDF()                                                
   functions +=
      "float SceneSDF(in vec3 point) {\n"
      "   return " + sdf + ";\n"
      "\n}\n\n";

   // Define the raymarching logic                                       
   functions += mSetup + "\n\n";
   functions +=
      "struct RaymarchResult {\n"
      "   float mDepth;\n"
      "};\n\n"

      "// This modified(!) implementation of Log-Bisect-Raymarching is"
      "// based on nimitz's original code, without his permission or endorsement:\n"
      "// https://www.shadertoy.com/view/4sSXzD \n"
      "// License: https://creativecommons.org/licenses/by-nc-sa/3.0/ \n"
      "float Bisect(in CameraResult camera, in float near, in float far) {\n"
      "   float mid = 0.0;\n"
      "   float sgn = sign(SceneSDF(camera.mDirection * near + camera.mOrigin));\n"
      "   for (int i = 0; i < 6; i++) {\n"
      "      mid = (near + far) * 0.5;\n"
      "      float d = SceneSDF(camera.mDirection * mid + camera.mOrigin);\n"
      "      if (abs(d) < iPrecision)\n"
      "         break;\n\n"

      "      d * sgn < 0.0 ? far = mid : near = mid;\n"
      "   }\n\n"

      "   return (near + far) * 0.5;\n"
      "}\n\n"

      "RaymarchResult Raymarch(in CameraResult camera) {\n"
      "   float t = 0.0;\n"
      "   float d = SceneSDF(camera.mDirection * t + camera.mOrigin);\n"
      "   float sgn = sign(d);\n"
      "   float told = t;\n"
      "   bool doBisect = false;\n"
      "   for (int i = 0; i <= iDetail; i++) {\n"
      "      if (abs(d) < iPrecision || t >= iFarMax)\n"
      "         break;\n"
      "      else if (i == iDetail)\n"
      "         t = iFarMax;\n\n"

      "      if (sign(d) != sgn) {\n"
      "         doBisect = true;\n"
      "         break;\n"
      "      }\n\n"

      "      told = t;\n"
      "      if (d > 1.0)\n"
      "         t += d * iFarStride;\n"
      "      else\n"
      "         t += log(abs(d) + 1.0 + iMinStep) * iBaseStride;\n\n"

      "      d = SceneSDF(camera.mDirection * t + camera.mOrigin);\n"
      "   }\n\n"

      "   if (doBisect)\n"
      "      t = Bisect(camera, told, t);\n\n"

      "   return RaymarchResult(t);\n"
      "}\n\n";
   Commit(ShaderToken::Functions, functions);

   GLSL usage =
      "RaymarchResult rmrResult = Raymarch(camResult);\n"
      "if (rmrResult.mDepth >= iFarMax)\n"
      "   discard;\n"
      "rmrResult.mDepth = 1.0 - rmrResult.mDepth / iFarMax;\n\n";
   Commit(ShaderToken::Transform, usage);
}

/// Generate the shader stages                                                
void MaterialNodeRaymarch::Generate() {
   PC_VERBOSE_MATERIAL("Generating code...");
   Descend();
   Consume();

   GenerateDefinition();
}
