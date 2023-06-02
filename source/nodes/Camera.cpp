///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Camera.hpp"
#include <Math/Matrices.hpp>

using namespace Nodes;


/// CameraResult shader structure                                             
constexpr Token CameraResult = R"shader(
   struct CameraResult {{
      vec2 mFragment;
      vec2 mScreenUV;
      vec3 mDirection;
      vec3 mOrigin;
      mat4 mProjectedView;
   }};
)shader";

/// PerPixel Camera() shader function                                         
///   @param {0} - resolution symbol (vec2)                                   
///   @param {1} - view transformation symbol (mat4)                          
///   @param {2} - field of view symbol (horizontal, in radians)              
///   @param {3} - projection transformation symbol (mat4)                    
constexpr Token CameraFuncPerPixel = R"shader(
   CameraResult Camera() {{
      CameraResult result;
      result.mFragment = vec2(gl_FragCoord.x, {0}.y - gl_FragCoord.y);
      result.mScreenUV = (result.mFragment * 2.0 - {0}) / {0}.x;
      result.mDirection = ({1} * normalize(vec4(result.mScreenUV, {2}, 0.0))).xyz;
      result.mOrigin = {1}[3].xyz;
      result.mProjectedView = {1}*{3};
      return result;
   }}
)shader";

/// PerVertex Camera() shader function                                        
///   @param {0} - view transformation (mat4)                                 
///   @param {1} - vertex position (vec4)                                     
constexpr Token CameraFuncPerVertex = R"shader(
   CameraResult Camera() {{
      const mat4 invView = invert({0});
      CameraResult result;
      gl_Position = result.mOrigin = invView * {1};
      result.mDirection = gl_Position - invView[3].xyz;
      return result;
   }}
)shader";

/// Default Camera() shader function (PerPixel)                               
///   @param {0} - resolution symbol (vec2)                                   
constexpr Token CameraFuncDefault = R"shader(
   CameraResult Camera() {{
      CameraResult result;
      result.mFragment = vec2(gl_FragCoord.x, {0}.y - gl_FragCoord.y);
      result.mScreenUV = (result.mFragment * 2.0 - {0}) / {0}.x;
      result.mDirection = vec3(0.0, 0.0, -1.0);
      result.mOrigin = vec3(result.mScreenUV, 0.0);
      const float uniformScale = 2.0 / {0}.x;
      result.mProjectedView = mat4(
          uniformScale, 0.0,              0.0,    0.0,
          0.0,          uniformScale,     0.0,    0.0,
          0.0,          0.0,              1.0,    0.0,
         -1.0,          -{0}.y / {0}.x,   1.0,    1.0
      );
      return result;
   }}
)shader";


/// Camera node descriptor-constructor                                        
///   @param desc - the camera node descriptor                                
Camera::Camera(const Descriptor& desc)
   : Node {MetaOf<Camera>(), desc} { }

/// Generate camera definition code, based on usage                           
void Camera::GenerateDefinition() {
   GLSL types;
   types += CameraResult;

   GLSL functions;
   bool explicitCamera = false;

   // Check traits in descriptor to figure out what kind of camera we   
   // are creating                                                      
   for (auto pair : mDescriptor.mTraits) {
      if (!pair.mKey->template Is<Traits::View>())
         continue;

      // Projection based on camera view transformation                 
      if (mRate == PerPixel) {
         auto symView   = GetSymbol<Traits::View, Mat4>(PerLevel);
         auto symFov    = GetSymbol<Traits::FOV, Real>(PerCamera);
         auto symProj   = GetSymbol<Traits::Projection, Mat4>(PerLevel);
         auto symRes    = GetSymbol<Traits::Size, Vec2>(PerTick);

         // Combine pixel position with the view matrix to from         
         // the projection per pixel. This allows for optically         
         // realistic rendering (near and far planes are not flat)      
         functions += TemplateFill(CameraFuncPerPixel, symRes, symView, symFov, symProj);
         explicitCamera = true;
      }
      else if (mRate == PerVertex) {
         auto symView   = GetSymbol<Traits::View, Mat4>(PerLevel);
         auto symPos    = GetSymbol<Traits::Place, Vec4>(PerVertex);

         // Combine vertex position with the view matrix to from        
         // the projection per vertex                                   
         functions += TemplateFill(CameraFuncPerVertex, symView, symPos);
         explicitCamera = true;
      }
      else TODO();
   }

   if (!explicitCamera) {
      // By default it simply projects 2D based on the pixel position   
      Logger::Warning("No explicit camera defined - using default 2D screen projection");
      mRate = PerPixel;
      auto symRes = GetSymbol<Traits::Size, Vec2>(PerTick);
      functions += TemplateFill(CameraFuncDefault, symRes);
   }

   GLSL constants = "CameraResult camResult = Camera();\n\n";

   Commit(ShaderToken::Functions, types + functions + constants);

   // Expose the results to the rest of the nodes                       
   Expose<Traits::Place, Vec2>("camResult.mFragment");
   Expose<Traits::Sampler, Vec2>("camResult.mScreenUV");
   Expose<Traits::Place, Vec3>("camResult.mOrigin");
   Expose<Traits::Projection, Mat4>("camResult.mProjectedView");
   Expose<Traits::Aim, Vec3>("camResult.mDirection");
}

/// Generate the shader stages                                                
void Camera::Generate() {
   VERBOSE_NODE("Generating code...");
   Descend();
   Consume();
   GenerateDefinition();
}
