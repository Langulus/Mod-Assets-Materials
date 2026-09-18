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
struct Material final : A::Material {
private:
   // Default material rate                                             
   RefreshRate mDefaultRate = Rate::Pixel;

   // Consumed bindings                                                 
   size_t mConsumedSamplers = 0;
   //size_t mConsumedLocations {};

   // Compiled flow                                                     
   Temporal mCompiled;

   // Defined symbols for each shader stage                             
   TUnorderedMap<GLSL, TMany<GLSL>> mDefinitions[ShaderStage::Counter];

   // Root node                                                         
   // It is of utmost importance this node is the last member, because  
   // it might use other members inside the Material, and those need to 
   // be initialized first                                              
   friend struct Nodes::Root;
   Nodes::Root mRoot;

public:
   LANGULUS(ABSTRACT) false;
   LANGULUS(PRODUCER) MaterialLibrary;
   LANGULUS_BASES(A::Material);
   LANGULUS_VERBS(Verbs::Create);

   Material(A::AssetModule*, const Many&);
   ~Material();

   void Create(Verb&);
   void Refresh() {}
   bool Generate(TMeta, size_t = 0);

   auto GetLOD(const LOD&) const -> Ref<A::Material>;
   auto GetDefaultRate() const noexcept -> RefreshRate;
   auto GetStage(size_t) -> GLSL&;
   auto GetStage(size_t) const -> GLSL const&;

   struct Stage {
      ShaderStage::Enum id;
      GLSL& code;
   };

   void ForEachStage(auto&&);
   void Commit   (RefreshRate, const Token&, const Token&);
   GLSL AddInput (RefreshRate, const Trait&, bool allowDuplicates);
   GLSL AddOutput(RefreshRate, const Trait&, bool allowDuplicates);
   void AddDefine(RefreshRate, const Token&, const GLSL&);

private:
   GLSL GenerateInputName (RefreshRate, const Trait&) const;
   GLSL GenerateOutputName(RefreshRate, const Trait&) const;
   void GenerateUniforms();
   void GenerateInputs();
   void GenerateOutputs();
   void InitializeFromShadertoy(const GLSL&);
};