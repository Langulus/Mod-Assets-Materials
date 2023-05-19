///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Camera.hpp"

using namespace Nodes;


/// Camera/projection node creation															
///	@param parent - the parent module													
///	@param verb - the camera creation verb												
Camera::Camera(MaterialNode* parent, const Verb& verb)
   : MaterialNode{ MetaOf<Camera>(), parent, verb } { }

/// Generate camera definition code, based on usage									
void Camera::GenerateDefinition() {
   GLSL types =
      "struct CameraResult {\n"
      "   vec2 mFragment;\n"
      "   vec2 mScreenUV;\n"
      "   vec3 mDirection;\n"
      "   vec3 mOrigin;\n"
      "   mat4 mProjectedView;\n"
      "};\n\n";

   // Check traits inside verb argument to figure out projection			
   GLSL functions;
   bool explicitCamera = false;
   mConstruct.GetAll().ForEach([&](const TraitID& trait) {
      if (trait != Traits::ViewTransform::ID)
         return;

      // Projection based on camera view transformation						
      if (mRate == RefreshRate::PerPixel) {
         auto viewSymbol = GetSymbol<Traits::ViewTransform, mat4>(RRate::PerLevel);
         auto fovSymbol = GetSymbol<Traits::FOV, real>(RRate::PerCamera);
         auto projectedViewSymbol = GetSymbol<Traits::ViewProjectTransform, mat4>(RRate::PerLevel);
         auto resolution = GetSymbol<Traits::Resolution, vec2>(RefreshRate::PerTick);

         // Combine pixel position with the view matrix to from			
         // the projection per pixel. This allows for optically			
         // realistic rendering (near and far planes are not flat)		
         functions =
            "CameraResult Camera() {\n"
            "   CameraResult result;\n"
            "   result.mFragment = vec2(gl_FragCoord.x, " + resolution + ".y - gl_FragCoord.y);\n"
            "   result.mScreenUV = (result.mFragment * 2.0 - " + resolution + ") / " + resolution + ".x;\n"
            "   result.mDirection = (" + viewSymbol + " * normalize(vec4(result.mScreenUV, " + fovSymbol + ", 0.0))).xyz; \n"
            "   result.mOrigin = " + viewSymbol + "[3].xyz; \n"
            "   result.mProjectedView = " + projectedViewSymbol + ";\n"
            "   return result;\n"
            "}\n\n";

         explicitCamera = true;
      }
      else if (mRate == RefreshRate::PerVertex) {
         auto viewSymbolInverse = GetSymbol<Traits::ViewTransformInverted, mat4>(RRate::PerLevel);
         auto posSymbol = GetSymbol<Traits::Position, mat4>(RRate::PerVertex);

         // Combine vertex position with the view matrix to from			
         // the projection per vertex												
         functions =
            "CameraResult Camera() {\n"
            "   CameraResult result;\n"
            "   gl_Position = result.mOrigin = " + viewSymbolInverse + " * " + posSymbol + "; \n"
            "   result.mDirection = gl_Position - " + viewSymbolInverse + "[3].xyz; \n"
            "   return result;\n"
            "}\n\n";

         explicitCamera = true;
      }
      else TODO();
   });

   if (!explicitCamera) {
      auto resolution = GetSymbol<Traits::Resolution, vec2>(RefreshRate::PerTick);

      // By default it simply projects 2D based on the pixel position	
      pcLogSelfWarning << 
         "No explicit camera defined - using default 2D screen projection";

      functions =
         "CameraResult Camera() {\n"
         "   CameraResult result;\n"
         "   result.mFragment = vec2(gl_FragCoord.x, " + resolution + ".y - gl_FragCoord.y);\n"
         "   result.mScreenUV = (result.mFragment * 2.0 - " + resolution + ") / " + resolution + ".x;\n"
         "   result.mDirection = vec3(0.0, 0.0, -1.0);\n"
         "   result.mOrigin = vec3(result.mScreenUV, 0.0);\n"
         "   const float uniformScale = 2.0 / " + resolution + ".x; \n"
         "   result.mProjectedView = mat4( \n"
         "      uniformScale, 0.0, 0.0, 0.0, \n"
         "      0.0, uniformScale, 0.0, 0.0, \n"
         "      0.0, 0.0, 1.0, 0.0, \n"
         "      -1.0, -" + resolution + ".y / " + resolution + ".x, 1.0, 1.0  \n"
         "   );\n\n"

         "   return result;\n"
         "}\n\n";
   }


   GLSL constants = "CameraResult camResult = Camera();\n\n";

   Commit(ShaderToken::Functions, types + functions + constants);

   // Expose the results to the rest of the nodes								
   Expose<Traits::Position, vec2>("camResult.mFragment");
   Expose<Traits::Sampler, vec2>("camResult.mScreenUV");
   Expose<Traits::Position, vec3>("camResult.mOrigin");
   Expose<Traits::ViewProjectTransform, mat4>("camResult.mProjectedView");
   Expose<Traits::Aim, vec3>("camResult.mDirection");
}

/// Generate the shader stages																
void Camera::Generate() {
   PC_VERBOSE_MATERIAL("Generating code...");
   Descend();
   Consume();

   GenerateDefinition();
}
