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
   ///   Fractal Brownian Motion node                                         
   ///                                                                        
   struct FBM final : Node {
      LANGULUS(ABSTRACT) false;
      LANGULUS_BASES(Node);
      LANGULUS_CONVERTS_TO(Text);

   private:
      // Code used to generate octaves                                  
      Code mCode;
      // Base weight value                                              
      Real mBaseWeight {0.5};
      // Number of octaves                                              
      Count mOctaveCount {2};

   public:
      FBM(Describe&&);

      const Symbol& Generate();
      operator Text() const;
   };

} // namespace Nodes


/// FBM function template                                                     
///   @param {0} - unique ID for the FBM function, used only in case of       
///                multiple FBM nodes in hierarchy                            
///   @param {1} - argument(s) for the FBM function                           
///   @param {2} - octave code                                                
constexpr Token FBMTemplate = R"shader(
   float FBM{0}(vec2 uv) {{
      const mat2 m = mat2(1.6, 1.2, -1.2, 1.6);
      float f = 0.0;
      {2}
      return f;
   }}
)shader";

/// FBM octave template                                                       
///   @param {0} - mass trait for the current octave                          
///   @param {1} - octave code to execute                                     
constexpr Token FBMOctave = R"shader(
      f += {0} * {1};
)shader";

/// FBM rotate template                                                       
constexpr Token FBMRotate = R"shader(
      uv = m * uv;
)shader";

/// FBM usage template                                                        
///   @param {0} - unique ID for the FBM function, used only in case of       
///                multiple FBM nodes in hierarchy                            
///   @param {1} - argument(s) for the FBM function                           
constexpr Token FBMUsage = R"shader(
      FBM{0}({1})
)shader";
