///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
///                                                                           
#pragma once
#include "Material.hpp"
#include <Langulus/Factory.hpp>
#include <Langulus/Verbs/Create.hpp>


///                                                                           
///   Material library                                                        
///                                                                           
/// Manages and produces Material(s)                                          
///                                                                           
struct MaterialLibrary final : Things::AssetModule {
   using CTTI_Abstract  = No;
   using CTTI_Bases     = Things::AssetModule;
   using CTTI_Ability   = Verbs::Create;

private:
   // Material factory                                                  
   TFactoryUnique<Material> mMaterials;
   // Data folder, where materials will be saved or loaded from         
   Ref<Things::Folder> mFolder;

public:
   MaterialLibrary(Runtime*, const Many&);

   void RequestGarbageCollection() {}

   void Create(Verb&);
   void Teardown();
};

