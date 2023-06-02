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
#include <Math/TInstance.hpp>


namespace Nodes
{

   ///                                                                        
   ///   Transform node                                                       
   ///                                                                        
   struct Transform : Node {
   private:
      // Keyframes                                                      
      TMap<Time, Verb> mKeyframes;

   public:
      Transform(const Descriptor&);

      void Generate() final;

      NOD() operator Debug() const;

   private:
      void GenerateDefinition();

      GLSL GetPosition(Offset, bool& runtime);
      GLSL GetScale(Offset, bool& runtime);
      GLSL GetAim(Offset, bool& runtime);
      GLSL GetInterpolator(Offset);
      GLSL GetTimer(Offset);
      TInstance<Vec3> GetInstance(Offset);
   };

} // namespace Nodes