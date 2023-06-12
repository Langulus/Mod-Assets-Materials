///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "../Node.hpp"
#include <Anyness/TMap.hpp>


namespace Nodes
{

   ///                                                                        
   ///   Texture node                                                         
   ///                                                                        
   /// Utilizes available sampler traits, sets up uniform samplers and loads  
   /// or generates static textures if required. Supports texture animations  
   /// of all kinds, solid colors, etc.                                       
   ///                                                                        
   struct Texture : Node {
   private:
      Temporal mKeyframes;
      Offset mTextureId {};

   public:
      Texture(const Descriptor&);

      Symbol Generate() final;

      NOD() operator Debug() const;

   private:
      GLSL GenerateKeyframe(const Temporal&);
   };

} // namespace Nodes