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
///	RAYCASTER MATERIAL NODE																	
///																									
class MaterialNodeRaycast : public MaterialNode {
   REFLECT(MaterialNodeRaycast);
public:
   MaterialNodeRaycast(MaterialNode*, const Verb&);
   MaterialNodeRaycast(MaterialNodeRaycast&&) noexcept = default;

public:
   void Generate() final;

private:
   void GenerateDefinition();
};