///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Material.hpp"
#include "MaterialLibrary.hpp"
#include <Anyness/Edit.hpp>


/// Material construction                                                     
///   @param producer - the producer                                          
///   @param descriptor - instructions for configuring the material           
Material::Material(A::AssetModule* producer, const Descriptor& descriptor)
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

/// Commit a code snippet to a specific stage and place                       
///	@param stage - the shader stage to commit to                            
///   @param place - the shader token to commit changes at                    
///   @param addition - the code to commit                                    
void Material::Commit(Offset stage, const Token& place, const GLSL& addition) {
   auto& code = GetStage(stage);
   if (code.IsEmpty()) {
      code += GLSL::Template(stage);
      VERBOSE_NODE("Added default template for ", ShaderStage::Names[stage]);
   }

   Edit(code).Select(place) >> addition;
   VERBOSE_NODE("Added code: ");
   VERBOSE_NODE(addition.Pretty());
}

/// Get a GLSL stage (const)                                                  
///   @param stage - the stage index                                          
///   @return the code associated with the stage                              
const GLSL& Material::GetStage(Offset stage) const {
   LANGULUS_ASSUME(DevAssumes, stage < ShaderStage::Counter,
      "Bad stage offset");
   return GetStages()[stage];
}

/// Get the list of GLSL stages                                               
///   @return the list                                                        
const TAny<GLSL>& Material::GetStages() const {
   auto stages = GetDataList<Traits::Data>();
   LANGULUS_ASSUME(DevAssumes, stages,
      "No data inside material");
   LANGULUS_ASSUME(DevAssumes, stages->GetCount() == ShaderStage::Counter,
      "Bad material data count");
   LANGULUS_ASSUME(DevAssumes, stages->template Is<GLSL>(),
      "Material data type mismatch");
   return *reinterpret_cast<const TAny<GLSL>*>(stages);
}

/// Add an external input trait                                               
///   @param rate - the rate at which the input will be refreshed             
///   @param traitOriginal - the input to add                                 
///   @param allowDuplicates - whether multiple such traits are allowed       
///   @return the generated symbol name                                       
GLSL Material::AddInput(Rate rate, const Trait& traitOriginal, bool allowDuplicates) {
   // An input must always be declared with a rate, and that rate       
   // must always be smaller or equal to the node's rate                
   // For example, you might want Time input PerPixel, but the          
   // actual uniform will be updated PerTick. So this is                
   // where we step in to override any wrongly provided rate            
   if (rate == Rate::Auto)
      rate = Node::DefaultTraitRate(traitOriginal.GetTrait());

   // Find any matching available inputs                                
   auto& inputs = GetDataAs<Traits::Trait, TAny<Trait>>(rate.GetInputIndex());
   if (!allowDuplicates) {
      auto found = inputs.Find(traitOriginal);
      if (!found.IsSpecial())
         return GenerateInputName(rate, inputs[found]);
   }
   
   // Add the new input                                                 
   auto trait = traitOriginal;
   if (trait.IsUntyped())
      trait.SetType(Node::DefaultTraitType(trait.GetTrait()), false);

   inputs << trait;
   const auto symbol = GenerateInputName(rate, trait);
   if (trait.TraitIs<Traits::Texture>())
      ++mConsumedSamplers;

   VERBOSE_NODE("Added input ", Logger::Cyan, trait, " as `", symbol, "` @ ", rate);
   return symbol;
}

/// Add an output to the material                                             
///   @param rate - the rate at which the output is refreshed                 
///   @param traitOriginal - the output to add                                
///   @param allowDuplicates - whether multiple such traits are allowed       
///   @return the generated symbol name                                       
GLSL Material::AddOutput(Rate rate, const Trait& traitOriginal, bool allowDuplicates) {
   if (!rate.IsShaderStage()) {
      throw Except::Content(pcLogSelfError
         << "Can't add non-output rate to material: " 
         << traitOriginal << " @ " << rate);
   }

   auto& outputs = GetDataAs<Traits::Trait, TAny<Trait>>(
      RRate::InputCount + rate.GetStageIndex());
   if (!allowDuplicates) {
      auto found = outputs.Find(traitOriginal);
      if (!found.IsSpecial())
         return GenerateOutputName(rate, outputs[found]);
   }

   // Add the new output                                                
   auto trait = traitOriginal;
   if (trait.IsUntyped())
      trait.SetType(Node::DefaultTraitType(trait.GetTrait()), false);

   outputs << trait;
   const auto symbol = GenerateOutputName(rate, trait);
   VERBOSE_NODE("Added output ", Logger::Cyan, trait, " as `", symbol, "` @ ", rate);
   return symbol;
}