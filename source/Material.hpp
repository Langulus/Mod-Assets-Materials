///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
///                                                                           
#pragma once
#include "nodes/Root.hpp"


///                                                                           
///   A material generator                                                    
///                                                                           
/// Capable of generating HLSL/GLSL code and meta data for shader compilation 
/// by parsing a material descriptor                                          
///                                                                           
struct Material final : Things::Material {
private:
   // Default material rate                                             
   RefreshRate mDefaultRate = Rate::Pixel;

   // Consumed bindings                                                 
   size_t mConsumedSamplers = 0;
   //size_t mConsumedLocations {};

   // Compiled flow                                                     
   Temporal mCompiled;

   // Defined symbols for each shader stage                             
   TMapUnsorted<GLSL, TMany<GLSL>> mDefinitions[ShaderStage::Counter];

   // Root node                                                         
   // It is of utmost importance this node is the last member, because  
   // it might use other members inside the Material, and those need to 
   // be initialized first                                              
   friend struct Nodes::Root;
   Nodes::Root mRoot;

public:
   using CTTI_Abstract = No;
   using CTTI_Producer = MaterialLibrary;
   using CTTI_Bases    = Things::Material;
   using CTTI_Ability  = Verbs::Create;

   Material(Things::AssetModule*, const Many&);
   ~Material();

   void Create(Verb&);
   void Refresh() {}
   bool Generate(TMeta, size_t = 0);

   auto GetLOD(const LOD&) const -> Ref<Things::Material>;
   auto GetDefaultRate() const noexcept -> RefreshRate;
   auto GetStage(size_t) -> GLSL&;
   auto GetStage(size_t) const -> GLSL const&;

   struct Stage {
      ShaderStage::Enum id;
      GLSL& code;
   };

   void ForEachStage(auto&&);
   void Commit   (RefreshRate, const Token&, const Token&);
   GLSL AddInput (RefreshRate, const Tag&, bool allowDuplicates);
   GLSL AddOutput(RefreshRate, const Tag&, bool allowDuplicates);
   void AddDefine(RefreshRate, const Token&, const GLSL&);

private:
   GLSL GenerateInputName (RefreshRate, const Tag&) const;
   GLSL GenerateOutputName(RefreshRate, const Tag&) const;
   void GenerateUniforms();
   void GenerateInputs();
   void GenerateOutputs();
   void InitializeFromShadertoy(const GLSL&);
};