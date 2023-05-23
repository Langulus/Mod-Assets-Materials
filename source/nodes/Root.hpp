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
///   ROOT MATERIAL NODE                                                      
///                                                                           
class MaterialNodeRoot : public MaterialNode {
   REFLECT(MaterialNodeRoot);
public:
   MaterialNodeRoot(CGeneratorMaterial*);
   MaterialNodeRoot(MaterialNodeRoot&&) noexcept = default;
   MaterialNodeRoot& operator = (MaterialNodeRoot&&) noexcept = default;

public:
   void Generate() final;

   NOD() operator Debug() const;
};