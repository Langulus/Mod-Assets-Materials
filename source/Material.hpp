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
   // Root node                                                         
   Nodes::Root mRoot;

   // Default material rate                                             
   Rate mDefaultRate = Rate::Auto;

   // Consumed bindings                                                 
   Count mConsumedSamplers {};
   Count mConsumedLocations {};

   // Compiled flow                                                     
   Temporal mCompiled;

   // Required definitions for each shader stage                        
   TAny<GLSL> mDefinitions[ShaderStage::Counter];

public:
   LANGULUS(ABSTRACT) false;
   LANGULUS_BASES(A::Material);
   LANGULUS_VERBS(Verbs::Create);

   Material(A::AssetModule*, const Descriptor&);

   void Create(Verb&);
   void Refresh();

   NOD() const A::Material* GetLOD(const LOD&) const;

   NOD() const Rate& GetDefaultRate() const noexcept;
   NOD() const GLSL& GetStage(Offset) const;
   NOD() const TAny<GLSL>& GetStages() const;

   void Commit(Offset, const Token&, const GLSL&);

   GLSL AddInput (Rate, const Trait&, bool allowDuplicates);
   GLSL AddOutput(Rate, const Trait&, bool allowDuplicates);
   GLSL AddDefine(Rate, const Token&, const GLSL&);
};