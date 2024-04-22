///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
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
   Count mConsumedSamplers = 0;
   //Count mConsumedLocations {};

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
   LANGULUS_BASES(A::Material);
   LANGULUS_VERBS(Verbs::Create);

   Material(A::AssetModule*, const Neat&);
   ~Material();

   void Create(Verb&);
   void Refresh() {}
   bool Generate(TMeta, Offset = 0);

   NOD() Ref<A::Material>  GetLOD(const LOD&) const;
   NOD() RefreshRate       GetDefaultRate() const noexcept;
   NOD() GLSL&             GetStage(Offset);
   NOD() GLSL const&       GetStage(Offset) const;

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
   NOD() GLSL GenerateInputName (RefreshRate, const Trait&) const;
   NOD() GLSL GenerateOutputName(RefreshRate, const Trait&) const;
   void GenerateUniforms();
   void GenerateInputs();
   void GenerateOutputs();
   void InitializeFromShadertoy(const GLSL&);
};