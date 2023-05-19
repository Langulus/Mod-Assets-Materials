///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "MaterialNode.hpp"


///																									
///	MATERIAL TRANSFORMATION NODE															
///																									
class MaterialNodeTransform : public MaterialNode {
   REFLECT(MaterialNodeTransform);
public:
   MaterialNodeTransform(MaterialNode*, const Verb&);
   MaterialNodeTransform(MaterialNodeTransform&&) noexcept = default;

public:
   void Generate() final;

   PC_VERB(Move);

   NOD() operator Debug() const;

private:
   void GenerateDefinition();

   GLSL GetPosition(pcptr idx, bool& runtime);
   GLSL GetScale(pcptr idx, bool& runtime);
   GLSL GetAim(pcptr idx, bool& runtime);
   GLSL GetInterpolator(pcptr idx);
   GLSL GetTimer(pcptr idx);
   InstanceReal<3> GetInstance(pcptr idx);

private:
   // Keyframes																			
   TMap<PCTime, Verb> mKeyframes;
};