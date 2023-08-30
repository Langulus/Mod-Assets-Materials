///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "Material.hpp"
#include <Flow/Verbs/Create.hpp>


///                                                                           
///   Material library                                                        
///                                                                           
/// Manages and produces Material(s)                                          
///                                                                           
struct MaterialLibrary final : A::AssetModule {
   LANGULUS(ABSTRACT) false;
   LANGULUS_BASES(A::AssetModule);
   LANGULUS_VERBS(Verbs::Create);

private:
   // Material factory                                                  
   TFactoryUnique<::Material> mMaterials;
   // Data folder, where materials will be saved or loaded from         
   Ptr<A::Folder> mFolder;

public:
   MaterialLibrary(Runtime*, const Neat&);

   void Update(Time);
   void Create(Verb&);
};

