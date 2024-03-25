///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "../Node.hpp"


namespace Nodes
{

   ///                                                                        
   ///   Texture node                                                         
   ///                                                                        
   /// Utilizes available sampler traits, sets up uniform samplers and loads  
   /// or generates static textures if required. Supports texture animations  
   /// of all kinds, solid colors, etc.                                       
   ///                                                                        
   struct Texture final : Node {
      LANGULUS(ABSTRACT) false;
      LANGULUS_BASES(Node);

   private:
      Temporal mKeyframes;
      Index mTextureId {IndexNone};
      Ref<A::Image> mTexture;

   public:
      Texture(Describe&&);

      void Detach();

      const Symbol& Generate();

   private:
      Ref<A::Image> CreateTexture(const Neat&);
      GLSL GenerateKeyframe(const Temporal&);
   };

} // namespace Nodes


/// Get pixel from sampler                                                    
///   @param {0} - sampler name                                               
///   @param {1} - texture coordinates                                        
constexpr Token GetPixelFunction = R"shader(
   texture({0}, {1})
)shader";

/// Texture flow function                                                     
///   @param {0} - first keyframe code                                        
///   @param {1} - intermediate keyframe branches                             
///   @param {2} - last keyframe code                                         
constexpr Token TextureFlowFunction = R"shader(
   vec4 TextureFlow(in float startTime, in float time, in vec2 uv) {{
      if (time <= startTime) {{
         // Time before/at start returns first keyframe
         return {0};
      }}
      {1}
      else {{
         // Time at/after last keyframe returns last keyframe
         return {2};
      }}
   }}
)shader";

/// Keyframe without texture transition                                       
///   @param {0} - frame end time                                             
///   @param {1} - GetPixelFunction function call                             
constexpr Token TextureFlowBranch = R"shader(
   else if (time < {0}) {{
      return {1};
   }}
)shader";

/// Keyframe with transition                                                  
///   @param {0} - frame end time                                             
///   @param {1} - frame start time                                           
///   @param {2} - frame length (end - start)                                 
///   @param {3} - GetPixelFunction function call from start side             
///   @param {4} - GetPixelFunction function call from end side               
constexpr Token TextureFlowBranchMix = R"shader(
   else if (time < {0}) {{
      const float ratio = (time - {1}) / {2};
      return mix({3}, {4}, ratio);
   }}
)shader";
