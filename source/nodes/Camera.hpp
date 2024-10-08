///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
///                                                                           
#pragma once
#include "../Node.hpp"


namespace Nodes
{

   ///                                                                        
   ///   Camera node                                                          
   ///                                                                        
   struct Camera final : Node {
      LANGULUS(ABSTRACT) false;
      LANGULUS_BASES(Node);

      Camera(Describe);
      auto Generate() -> const Symbol&;
   };

} // namespace Nodes


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

