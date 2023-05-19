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
///   User interface module using ImGUI                                       
///                                                                           
/// Manages and produces GUI systems                                          
///                                                                           
struct MaterialLibrary final : A::AssetsModule {
   LANGULUS(ABSTRACT) false;
   LANGULUS_BASES(A::AssetsModule);
   LANGULUS_VERBS(Verbs::Create);

private:
   // Material factory                                                  
   TFactoryUnique<Material> mMaterials;
   // Data folder, where materials will be saved or loaded from         
   Ptr<A::Folder> mFolder;

public:
   MaterialLibrary(Runtime*, const Descriptor&);

   void Update(Time);
   void Create(Verb&);
};

