///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Material.hpp"
#include "MaterialLibrary.hpp"


/// Material construction                                                     
///   @param producer - the producer                                          
///   @param descriptor - instructions for configuring the material           
Material::Material(MaterialLibrary* producer, const Descriptor& descriptor)
   : A::Material {MetaOf<Material>(), producer, descriptor}
   , mRoot {this, descriptor} {
   // Extract default rate if any                                       
   if (!mDescriptor.ExtractTrait<Traits::Rate>(mDefaultRate))
      mDescriptor.ExtractData(mDefaultRate);
}

/// Get default material rate                                                 
///   @return the rate                                                        
const Rate& Material::GetDefaultRate() const noexcept {
   return mDefaultRate;
}
