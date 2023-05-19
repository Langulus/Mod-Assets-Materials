///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "MaterialNode.hpp"

///																									
///	INPUT MATERIAL NODE																		
///																									
class MaterialNodeValue : public MaterialNode {
   Trait mTrait;
   GLSL mUniform, mName, mUse;
   GLSL mDependencies;

public:
   REFLECT(MaterialNodeValue);
   MaterialNodeValue(CGeneratorMaterial*);
   //MaterialNodeValue(const MaterialNodeValue&) = default;
   //MaterialNodeValue(MaterialNodeValue&&) noexcept = default;
   MaterialNodeValue(MaterialNode*, const Verb& = {});

   NOD() static MaterialNodeValue Input(CGeneratorMaterial*, const Trait&, RRate = RRate::PerAuto, const GLSL& name = {});
   NOD() static MaterialNodeValue Output(CGeneratorMaterial*, const Trait&, RRate = RRate::PerAuto, const GLSL& name = {});
   NOD() static MaterialNodeValue Input(MaterialNode*, const Trait&, RRate = RRate::PerAuto, const GLSL& name = {});
   NOD() static MaterialNodeValue Output(MaterialNode*, const Trait&, RRate = RRate::PerAuto, const GLSL& name = {});
   NOD() static MaterialNodeValue Local(MaterialNode*, const Trait&, RRate = RRate::PerAuto, const GLSL& name = {});

   //MaterialNodeValue& operator = (const MaterialNodeValue&) = default;
   //MaterialNodeValue& operator = (MaterialNodeValue&&) = default;

public:
   void Generate() override;

   PC_VERB(Project);
   PC_VERB(Select);
   PC_VERB(Add);
   PC_VERB(Multiply);
   PC_VERB(Modulate);
   PC_VERB(Exponent);
   PC_VERB(Randomize);
   PC_VERB(FBM);

   void DoGASM(const GASM&);
   NOD() GLSL SelectMember(TraitID, Trait&);
   NOD() GLSL GetDeclaration() const;

   NOD() inline TMeta GetTrait() const noexcept {
      return mTrait.GetTraitMeta();
   }

   void BindTo(const Trait&, const MaterialNode*);

   inline bool IsValid() const {
      return !mName.IsEmpty();
   }

   NOD() operator Debug() const;

private:
   void AutoCompleteTrait();
};