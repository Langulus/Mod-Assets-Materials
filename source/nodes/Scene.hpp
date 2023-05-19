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
///	MATERIAL SCENE NODE																		
///																									
class MaterialNodeScene : public MaterialNode {
   REFLECT(MaterialNodeScene);
public:
   MaterialNodeScene(MaterialNode*, const Verb&);
   MaterialNodeScene(MaterialNodeScene&&) noexcept = default;

public:
   void Generate() final;

   PC_VERB(Move);
   PC_VERB(Texturize);

   void GenerateSDFCode(GLSL& define, GLSL& scene);
   void GenerateTriangleCode(GLSL& define, GLSL& sceneTriangles, pcptr& triangleCount);
   void GenerateLineCode(GLSL& define, GLSL& sceneLines, pcptr& lineCount);

   NOD() operator Debug() const;

private:
   Ptr<CGeneratorGeometry> mGeometry;
};