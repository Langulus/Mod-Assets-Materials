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
///   TEXTURIZER NODE                                                         
///                                                                           
/// Utilizes available sampler traits, sets up uniform samplers and loads      
/// or generates static textures if required. Supports texture animations of   
/// all kinds, solid colors, etc.                                             
///                                                                           
class MaterialNodeTexture : public MaterialNode {
   REFLECT(MaterialNodeTexture);
public:
   MaterialNodeTexture(MaterialNode*, const Verb&);
   MaterialNodeTexture(MaterialNodeTexture&&) noexcept = default;

public:
   void Generate() final;

   PC_VERB(Texturize);

   NOD() operator Debug() const;

private:
   using KeyframeMap = TMap<PCTime, Verb>;
   GLSL GenerateDefinition(KeyframeMap*, const GLSL&);
   GLSL GetKeyframe(KeyframeMap*, pcptr, const GLSL&);
   GLSL GetTextureCoordinates();

private:
   // Supports animating multiple texture channels                        
   // Index zero and one are reserved for front/back of polygons         
   TMap<pcptr, KeyframeMap> mKeyframes;
   KeyframeMap mKeyframesGlobal;
};