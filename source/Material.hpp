///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
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
   Rate mDefaultRate {PerPixel};

   // Consumed bindings                                                 
   Count mConsumedSamplers {};
   Count mConsumedLocations {};

   // Compiled flow                                                     
   Temporal mCompiled;

   // Defined symbols for each shader stage                             
   TUnorderedMap<GLSL, TAny<GLSL>> mDefinitions[ShaderStage::Counter];

   // Root node                                                         
   // It is of utmost importance this node is the last member, because  
   // it might use other members inside the Material, and those need to 
   // be initialized first                                              
   Nodes::Root mRoot;

public:
   LANGULUS(ABSTRACT) false;
   LANGULUS_BASES(A::Material);
   LANGULUS_VERBS(Verbs::Create);

   Material(A::AssetModule*, const Descriptor&);

   void Create(Verb&);
   void Refresh() {}

   NOD() const A::Material* GetLOD(const LOD&) const;

   NOD() const Rate& GetDefaultRate() const noexcept;
   NOD()       GLSL& GetStage(Offset);
   NOD() const GLSL& GetStage(Offset) const;
   NOD()       TAny<GLSL>& GetStages();
   NOD() const TAny<GLSL>& GetStages() const;

   void Commit(Rate, const Token&, const Token&);

   GLSL AddInput (Rate, const Trait&, bool allowDuplicates);
   GLSL AddOutput(Rate, const Trait&, bool allowDuplicates);
   void AddDefine(Rate, const Token&, const GLSL&);

private:
   NOD() GLSL GenerateInputName(Rate, const Trait&) const;
   NOD() GLSL GenerateOutputName(Rate, const Trait&) const;
   void GenerateUniforms();
   void GenerateInputs();
   void GenerateOutputs();
   void InitializeFromShadertoy(const GLSL&);
};