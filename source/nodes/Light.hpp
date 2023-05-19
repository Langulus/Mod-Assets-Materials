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
///	LIGHT MATERIAL NODE																		
///																									
class MaterialNodeLight : public MaterialNode {
   REFLECT(MaterialNodeLight);
public:
   MaterialNodeLight(MaterialNode*, const Verb&);
   MaterialNodeLight(MaterialNodeLight&&) noexcept = default;

public:
   void Generate() final;

   PC_VERB(Illuminate);

private:
   void GenerateDefinition();
   void GenerateUsage();
};