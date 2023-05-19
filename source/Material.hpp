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
struct MaterialNode final : A::Unit, ProducedFrom<Material> {
   LANGULUS(ABSTRACT) false;
   LANGULUS(PRODUCER) Material;
   LANGULUS_BASES(A::Unit);
   LANGULUS_VERBS(Verbs::Create);

private:
   // Root node																			
   MaterialNodeRoot mRoot;

   // Consumed bindings																	
   pcptr mConsumedSamplers = 0;
   pcptr mConsumedLocations = 0;

   // Compiled GASM code																
   CFlow mCompiled;

public:
   MaterialNode(Material*, const Descriptor&);
   ~MaterialNode();

   void Create(Verb&);
   void Refresh();
};