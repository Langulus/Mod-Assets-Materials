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
///	RAYTRACE MATERIAL NODE																	
///																									
class MaterialNodeRaytrace : public MaterialNode {
   REFLECT(MaterialNodeRaytrace);
public:
   MaterialNodeRaytrace(MaterialNode*, const Verb&);
   MaterialNodeRaytrace(MaterialNodeRaytrace&&) noexcept = default;

public:
   void Generate() final;

private:
   void GenerateDefinition();
};