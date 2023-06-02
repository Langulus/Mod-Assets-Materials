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
      // Supports animating multiple texture channels                   
      // Index zero and one are reserved for front/back of polygons     
      using KeyframeMap = TMap<Time, Verb>;
      TMap<Offset, KeyframeMap> mKeyframes;
      KeyframeMap mKeyframesGlobal;

   public:
      Texture(const Descriptor&);

      void Generate() final;

      NOD() operator Debug() const;

   private:
      GLSL GenerateDefinition(KeyframeMap*, const GLSL&);
      GLSL GetKeyframe(KeyframeMap*, Offset, const GLSL&);
      GLSL GetTextureCoordinates();
   };

} // namespace Nodes