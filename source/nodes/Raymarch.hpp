///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "MaterialNodeScene.hpp"


///                                                                           
///   RAYMARCHER MATERIAL NODE                                                
///                                                                           
class MaterialNodeRaymarch : public MaterialNode {
   REFLECT(MaterialNodeRaymarch);
public:
   MaterialNodeRaymarch(MaterialNode*, const Verb&);
   MaterialNodeRaymarch(MaterialNodeRaymarch&&) noexcept = default;

public:
   void Generate() final;

private:
   void GenerateDefinition();

private:
   GLSL mSetup;
};