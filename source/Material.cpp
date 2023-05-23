///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Material.hpp"


/// Material construction                                                     
///   @param producer - the producer                                          
///   @param descriptor - instructions for configuring the material           
Material::Material(MaterialLibrary* producer, const Descriptor& descriptor)
   : A::Material {MetaOf<Material>(), producer, descriptor} {

}

/// GUI system destruction                                                    
Material::~Material() {

}
