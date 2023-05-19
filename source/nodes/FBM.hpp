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
///	FRACTAL BROWNIAN MOTION MATERIAL NODE												
///																									
class MaterialNodeFBM : public MaterialNode {
   REFLECT(MaterialNodeFBM);
public:
   MaterialNodeFBM(MaterialNode*);
   MaterialNodeFBM(MaterialNodeFBM&&) noexcept = default;

public:
   void Generate() final;

   PC_VERB(FBM);

   NOD() operator Debug() const;

private:
   void GenerateDefinition();
   void GenerateUsage();

private:
   GASM mCodePerOctave;
   GLSL mCodePerUse;
   Trait mArgument;
   pcptr mOctaveCount = 1;
};