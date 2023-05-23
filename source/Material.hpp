///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "Node.hpp"


///                                                                           
///   GUI system                                                              
///                                                                           
/// Manages and produces GUI items that interact with each other within an    
/// isolated system                                                           
///                                                                           
struct Material final : A::Material {
   LANGULUS(ABSTRACT) false;
   LANGULUS(PRODUCER) MaterialLibrary;
   LANGULUS_BASES(A::Material);
   LANGULUS_VERBS(Verbs::Create);

private:
   // Root node																			
   MaterialNodeRoot mRoot;

   // Consumed bindings																	
   pcptr mConsumedSamplers = 0;
   pcptr mConsumedLocations = 0;

   // Compiled GASM code																
   Temporal mCompiled;

public:
   Material(MaterialLibrary*, const Descriptor&);
   ~Material();

   void Create(Verb&);
   void Refresh();

   NOD() const A::Material* GetLOD(const Math::LOD&) const;
};