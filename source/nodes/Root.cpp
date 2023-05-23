///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "../MContent.hpp"


/// Root node creation                                                         
///   @param producer - the producer material                                 
MaterialNodeRoot::MaterialNodeRoot(CGeneratorMaterial* producer)
   : MaterialNode{ MetaData::Of<MaterialNodeRoot>(), producer } { }

/// For logging                                                               
MaterialNodeRoot::operator Debug() const {
   GASM result;
   result += DataID::Of<ME>;
   result += GASM::OpenScope;
   result += GASM::CloseScope;
   return result;
}

/// Generate the shader stages                                                
void MaterialNodeRoot::Generate() {
   PC_VERBOSE_MATERIAL("Generating code...");
   Descend();
   Consume();
}
