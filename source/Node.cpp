///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "../MContent.hpp"

/// Material node construction																
///	@param classid - the node type														
///	@param producer - the producer														
MaterialNode::MaterialNode(DMeta classid, CGeneratorMaterial* producer)
   : AContext {classid}
   , TProducedFrom {producer}
   , mRate {producer->GetDefaultRate()} { }

/// Material node construction																
///	@param classid - the node type														
///	@param parent - the parent node														
///	@param creator - the creation verb containing node description				
MaterialNode::MaterialNode(DMeta classid, MaterialNode* parent, const Verb& creator)
   : AContext {classid}
   , TProducedFrom {parent->GetProducer()}
   , mRate {Rate(creator, parent->GetProducer()->GetDefaultRate())} {
   mConstruct << creator.GetArgument();
   mParent = parent;
}

/// Get content manager																			
///	@return a pointer to the manager														
MContent* MaterialNode::GetContentManager() const noexcept {
   return mProducer->GetProducer();
}

/// Get list of owners																			
///	@return the material owners															
const TAny<Entity*>& MaterialNode::GetOwners() const noexcept {
   return mProducer->GetOwners();
}

/// Introduce new nodes																			
///	@param verb - the selection verb														
void MaterialNode::Create(Verb& verb) {
   verb.GetArgument().ForEachDeep([&](const Block& group) {
      EitherDoThis
         group.ForEach([&](const DataID& content) {
            PC_VERBOSE_MATERIAL("Adding scene node: " << content);
            auto scene = EmplaceChildUnique(MaterialNodeScene(this, verb));
            verb << scene;
         })
      OrThis
         group.ForEach([&](const Construct& content) {
            PC_VERBOSE_MATERIAL("Adding scene node: " << content);
            auto scene = EmplaceChildUnique(MaterialNodeScene(this, verb));
            verb << scene;
         });
   });
}

/// Interface inputs/outputs in node hierarchy											
///	@param verb - the selection verb														
void MaterialNode::Select(Verb& verb) {
   const auto rate = MaterialNode::Rate(verb, mRate);
   verb.GetArgument().ForEachDeep([&](const Block& group) {
      EitherDoThis
         group.ForEach([&](const TraitID& trait) {
            // Search for an output from parents, scanning the				
            // hierarchy. If no such symbol was found, a new node			
            // will be created														
            auto foundOrCreated = 
               GetValue(trait.GetMeta(), nullptr, rate, true);
            verb << foundOrCreated;
         })
      OrThis
         group.ForEach([&](const Trait& trait) {
            // Search for an output from parents, scanning the				
            // hierarchy. If no such symbol was found, a new node			
            // will be created														
            auto foundOrCreated = 
               GetValue(trait.GetTraitMeta(), trait.GetMeta(), rate, true);
            verb << foundOrCreated;
         });
   });
}

/// Calculate and check rate from a verb frequency										
///	@param verb - the verb to analyze													
///	@param fallback - rate that will be used if verb is PerAuto					
///	@return the refresh rate																
RRate MaterialNode::Rate(const Verb& verb, RRate fallback) {
   const RRate::Enum fromVerb = static_cast<RRate::Enum>(verb.GetFrequency());
   switch (fromVerb) {
   case RRate::PerAuto:
      return fallback;
   case RRate::PerPixel:
   case RRate::PerVertex:
      return fromVerb;
   default:
      if (fromVerb < RRate::PerVertex) {
         // Projection provided as a uniform (or constant)					
         return fromVerb;
      }

      // Error condition																
      throw Except::Content(pcLogError
         << "Unsupported rate: " << verb.GetFrequency());
   }
}

/// Get stage from node rate																	
///	@return the shader stage that will be used										
ShaderStage::Enum MaterialNode::GetStage() const {
   auto result = mRate.GetStageIndex();
   if (result != ShaderStage::Counter)
      return result;

   if (mParent) {
      const auto stage = mParent->GetStage();
      if (stage != ShaderStage::Counter)
         return stage;
   }

   return ShaderStage::Counter;
}

/// Generate all children's code																
void MaterialNode::Descend() {
   for (auto child : mChildren)
      child->Generate();
}

/// Commit a code snippet to a specific stage and place								
///	@param place - the shader token to commit changes at							
///	@param addition - the code to commit												
void MaterialNode::Commit(ShaderToken::Enum place, const GLSL& addition) {
   GetProducer()->Commit(GetStage(), place, addition);
}

/// Log the material node hierarchy															
void MaterialNode::Dump() const {
   pcLogVerbose << this;
   if (mChildren.IsEmpty())
      return;

   pcLog << " {";
   pcLog << ccTab;
   for (auto child : mChildren)
      child->Dump();
   pcLog << ccUntab;
   pcLogVerbose << "}";
}

Debug MaterialNode::DebugBegin() const {
   GASM result;
   result += ClassMeta()->GetToken();
   result += GASM::Frequency;
   result += mRate;
   result += GASM::OpenScope;
   return result;
}

Debug MaterialNode::DebugEnd() const {
   GASM result;
   result += GASM::CloseScope;
   return result;
}

/// For logging																					
MaterialNode::operator Debug() const {
   GASM result;
   result += DebugBegin();
   result += DebugEnd();
   return result;
}

/// Convert node's output to GLSL code														
///	@return GLSL code equivalent to the node's output								
MaterialNode::operator GLSL() const {
   return mOutputs.GetValue(0);
}

/// Generate GLSL code from a construct													
///	@param construct - the construct to interpret as GLSL							
///	@return the GLSL code																	
GLSL MaterialNode::CodeFromConstruct(const Construct& construct) {
   // Attempt static construction													
   Any created;
   try {
      construct.StaticCreation(created);
      return GLSL{ created };
   }
   catch (const Except::BadConstruction&) { }

   // Propagate the construct as GLSL												
   Any context { mParent };
   auto code = GLSL::Type(construct.GetMeta()) + "(";
   bool separator = false;
   construct.GetAll().ForEachDeep([&](const Block& group) {
      if (separator)
         code += ", ";

      if (group.Is<Verb>()) {
         // Execute verbs in parent scope if any							
         Any scopeResults;
         Any scope{ group };
         Verb::ExecuteScope(context, scope, scopeResults);
         code += scopeResults;
      }
      else code += group;

      separator = true;
   });

   code += ")";
   return code;
}

/// Generate GLSL code from a scope															
///	@param scope - the scope to interpret as GLSL									
///	@return the GLSL code																	
GLSL MaterialNode::CodeFromScope(const Any& scope) {
   GLSL code;
   Any scopeLocal { scope };
   Any scopeResults;
   Any context { mParent };
   Verb::ExecuteScope(context, scopeLocal, scopeResults);
   scopeResults.ForEach([&code](const MaterialNode& node) {
      code += node.GetOutputSymbol();
   });
   return code;
}

/// Check if a node already exists in parent chain										
///	@param what - what node to search for												
///	@return true if any of the parents match the search							
bool MaterialNode::IsInHierarchy(MaterialNode* what) const {
   return mParent && (mParent == what || mParent->IsInHierarchy(what));
}

/// Add child, also sets parents for child												
///	@param child - child pointer to add													
void MaterialNode::AddChild(MaterialNode* child) {
   if (this == child)
      return;
   if (mChildren.Find(child) != uiNone)
      return;
   if (IsInHierarchy(child))
      return;

   child->mParent = this;
   mChildren << child;
   PC_VERBOSE_MATERIAL("Added child " << ccCyan << child);
}

/// Remove child, also removes parent of child											
///	@param child - child pointer to remove												
void MaterialNode::RemoveChild(MaterialNode* child) {
   auto found = mChildren.Find(child);
   if (found.IsSpecial())
      return;

   child->mParent.Reset();
   mChildren.RemoveIndex(static_cast<pcptr>(found));
   PC_VERBOSE_MATERIAL("Removed child " << ccCyan << child);
}

/// Return the first output trait															
///	@return trait																				
const Trait& MaterialNode::GetOutputTrait() const {
   if (mOutputs.IsEmpty())
      throw Except::Content(pcLogSelfError << "No default output available");
   return mOutputs.GetKey(0);
}

/// Search and return an output trait, if any											
///	@param trait - the trait to search for in outputs								
///	@return the symbol or an empty container if nothing was found				
Trait MaterialNode::GetOutputTrait(const Trait& trait) const {
   for (pcptr i = 0; i < mOutputs.GetCount(); ++i) {
      auto& key = mOutputs.GetKey(i);
      if (trait.IsSimilar(key))
         return key;
   }

   return {};
}

/// Search and return an output symbol, if any											
///	@param trait - the trait to search for in outputs								
///	@return the symbol or an empty container if nothing was found				
GLSL MaterialNode::GetOutputSymbol(const Trait& trait) const {
   for (pcptr i = 0; i < mOutputs.GetCount(); ++i) {
      if (trait.IsSimilar(mOutputs.GetKey(i)))
         return mOutputs.GetValue(i);
   }

   return {};
}

/// Return the first output symbol															
///	@return the symbol																		
const GLSL& MaterialNode::GetOutputSymbol() const {
   if (mOutputs.IsEmpty())
      throw Except::Content(pcLogSelfError << "Node has no outputs");
   return mOutputs.GetValue(0);
}

/// Convert a symbol from one type to another											
///	@param trait - the trait to convert													
///	@param symbol - the symbol for the original trait								
///	@param as - the type to convert to													
///	@param filler - number for filling empty stuff									
///	@return the new symbol																	
GLSL ConvertSymbol(const Trait& trait, const GLSL& symbol, DMeta as, real filler) {
   auto from = trait.GetMeta();
   if (from->InterpretsAs(as))
      return symbol;
   
   // Matrix types																		
   if (from->InterpretsAs<TSizedMatrix<4>>()) {
      if (as->InterpretsAs<TSizedMatrix<3>>())
         return "mat3(" + symbol + ")";
      else if (as->InterpretsAs<TSizedMatrix<2>>())
         return "mat2(" + symbol + ")";
   }
   else if (from->InterpretsAs<TSizedMatrix<3>>()) {
      if (as->InterpretsAs<TSizedMatrix<4>>())
         return "mat4(" + symbol + "[0], 0.0, " + symbol + "[1], 0.0, " + symbol + "[2], 0.0, " + symbol + "[3], " + filler + ")";
      else if (as->InterpretsAs<TSizedMatrix<2>>())
         return "mat2(" + symbol + ")";
   }
   else if (from->InterpretsAs<TSizedMatrix<2>>()) {
      if (as->InterpretsAs<TSizedMatrix<4>>())
         return "mat4(" + symbol + "[0], 0.0, 0.0, " + symbol + "[1], 0.0, 0.0, " + symbol + "[2], " + filler + ", 0.0, " + symbol + "[3], 0.0, " + filler + ")";
      else if (as->InterpretsAs<TSizedMatrix<3>>())
         return "mat4(" + symbol + "[0], 0.0, " + symbol + "[1], 0.0, " + symbol + "[2], 0.0, " + symbol + "[3], " + filler + ")";
   }
   // Vector types																		
   else if (from->InterpretsAs<TSizedVector<4>>()) {
      if (as->InterpretsAs<TSizedVector<4>>())
         return symbol;
      else if (as->InterpretsAs<TSizedVector<3>>())
         return symbol + ".xyz";
      else if (as->InterpretsAs<TSizedVector<2>>())
         return symbol + ".xy";
      else if (as->InterpretsAs<TSizedVector<1>>() || as->InterpretsAs<ANumber>())
         return symbol + ".x";
   }
   else if (from->InterpretsAs<TSizedVector<3>>()) {
      if (as->InterpretsAs<TSizedVector<4>>())
         return "vec4(" + symbol + ", 1.0)";
      else if (as->InterpretsAs<TSizedVector<3>>())
         return symbol;
      else if (as->InterpretsAs<TSizedVector<2>>())
         return symbol + ".xy";
      else if (as->InterpretsAs<TSizedVector<1>>() || as->InterpretsAs<ANumber>())
         return symbol + ".x";
   }
   else if (from->InterpretsAs<TSizedVector<2>>()) {
      if (as->InterpretsAs<TSizedVector<4>>())
         return "vec4(" + symbol + ", " + filler + ", 1.0)";
      else if (as->InterpretsAs<TSizedVector<3>>())
         return "vec3(" + symbol + ", " + filler + ")";
      else if (as->InterpretsAs<TSizedVector<2>>())
         return symbol;
      else if (as->InterpretsAs<TSizedVector<1>>() || as->InterpretsAs<ANumber>())
         return symbol + ".x";
   }
   else if (from->InterpretsAs<TSizedVector<1>>() || from->InterpretsAs<ANumber>()) {
      if (as->InterpretsAs<TSizedVector<4>>())
         return "vec4(vec3(" + symbol + "), 1.0)";
      else if (as->InterpretsAs<TSizedVector<3>>())
         return "vec3(" + symbol + ")";
      else if (as->InterpretsAs<TSizedVector<2>>())
         return "vec2(" + symbol + ")";
      else if (as->InterpretsAs<TSizedVector<1>>() || as->InterpretsAs<ANumber>())
         return symbol;
   }

   throw Except::Content(pcLogFuncError 
      << "Can't reinterpret GLSL type " << trait << " to " << as->GetToken());
}

/// Return the first output symbol, converted as the desired type					
///	@param as - the type we want to convert to										
///	@param filler - value to fill expanded parts (if any)							
///	@return the symbol																		
GLSL MaterialNode::GetOutputSymbolAs(DMeta as, real filler) const {
   if (mOutputs.IsEmpty())
      throw Except::Content(pcLogSelfError << "Node has no outputs");
   return ConvertSymbol(mOutputs.GetKey(0), mOutputs.GetValue(0), as, filler);
}

/// Return the matching output symbol, converted as the desired type				
///	@param trait - the trait we're searching for										
///	@param as - the type we want to convert to										
///	@param filler - value to fill expanded parts (if any)							
///	@return the symbol																		
GLSL MaterialNode::GetOutputSymbolAs(const Trait& trait, DMeta as, real filler) const {
   for (pcptr i = 0; i < mOutputs.GetCount(); ++i) {
      if (trait.IsSimilar(mOutputs.GetKey(i)))
         return ConvertSymbol(mOutputs.GetKey(i), mOutputs.GetValue(i), as, filler);
   }

   return {};
}

/// Get (or make) node that contains an input we're searching for					
///	@param trait - the trait to search for (nullptr for any)						
///	@param rate - the rate of the trait to use										
///	@param checkHere - whether or not to check local outputs						
///	@return the symbol and usage															
bool MaterialNode::InnerGetValue(const Trait& trait, RRate rate, bool checkHere, MaterialNodeValue& output) const {
   // Check here																			
   if (checkHere) {
      PC_VERBOSE_MATERIAL("Searching for " << trait << " inside " << *this);
      for (auto& key : mOutputs.Keys()) {
         if (!trait.IsSimilar(key))
            continue;

         output.BindTo(key, this);
         return true;
      }
   }

   if (mParent) {
      // Scan previous node's outputs												
      auto thisIndex = mParent->mChildren.Find(this);
      const auto start = thisIndex == uiNone 
         ? mParent->mChildren.GetCount() 
         : pcptr(thisIndex);

      for (pcptr i = start; i > 0; --i) {
         // Check outputs of all children up to this one						
         auto child = mParent->mChildren[i - 1];
         PC_VERBOSE_MATERIAL("Searching for " << trait << " inside " << *child);
         for (auto& key : child->mOutputs.Keys()) {
            if (!trait.IsSimilar(key))
               continue;

            output.BindTo(key, child);
            return true;
         }
      }

      // Then check the parent itself												
      PC_VERBOSE_MATERIAL("Searching for " << trait << " inside " << *mParent);
      for (auto& key : mParent->mOutputs.Keys()) {
         if (!trait.IsSimilar(key))
            continue;

         output.BindTo(key, mParent);
         return true;
      }

      // Climb the hierarchy in search of more									
      for (pcptr i = start; i > 0; --i) {
         auto child = mParent->mChildren[i - 1];
         if (child->InnerGetValue(trait, rate, false, output))
            return true;
      }
   }

   return false;
}

/// Get (or make) node that contains an input we're searching for					
///	@param tmeta - the trait to search for (nullptr for any)						
///	@param dmeta - the data to search for (nullptr for any)						
///	@param rate - the rate of the trait to use										
///	@param addIfMissing - whether or not to generate default usage				
///	@return the symbol and usage															
MaterialNodeValue MaterialNode::GetValue(TMeta tmeta, DMeta dmeta, RRate rate, bool addIfMissing) {
   if (rate == RRate::PerAuto)
      rate = mRate;

   const auto trait = Trait::FromMeta(tmeta, dmeta);
   auto result = MaterialNodeValue::Local(this, trait, rate);
   if (InnerGetValue(trait, rate, true, result)) {
      if (result.GetRate() < rate) {
         // We have to build a bridge to this rate								
         mProducer->AddOutput(result.GetRate(), result.GetOutputTrait(), false);
         auto newName = mProducer->AddInput(rate, result.GetOutputTrait(), false);
         result.mRate = rate;
         result.mOutputs.Values()[0] = newName;
      }

      return result;
   }

   // Undefined symbol																	
   if (addIfMissing) {
      pcLogSelfWarning 
         << "No input available, so adding default one: " 
         << GetProducer()->GenerateInputName(rate, trait);
      result.BindTo(trait, nullptr);
   }

   return result;
}

/// Get input symbol from nodes																
///	@param tmeta - the trait to search for (nullptr for any)						
///	@param dmeta - the data to search for (nullptr for any)						
///	@param rate - the rate of the trait to use										
///	@param addIfMissing - whether or not to generate default usage				
///	@return the symbol and usage															
GLSL MaterialNode::GetSymbol(TMeta tmeta, DMeta dmeta, RRate rate, bool addIfMissing) {
   auto found = GetValue(tmeta, dmeta, rate, addIfMissing);
   const auto trait = Trait::FromMeta(tmeta, dmeta);
   auto symbol = found.GetOutputSymbol(trait);
   if (symbol.IsEmpty()) {
      PC_VERBOSE_MATERIAL(ccRed << "Undefined input " << tmeta->GetToken()
         << " (of type " << (dmeta ? dmeta->GetToken() : LiteralText {"any"}) << ") @ " << rate);
      return {};
   }

   return symbol;
}

/// Get a default type of traits																
///	@param trait - the trait definition													
///	@return the associated data type														
DMeta MaterialNode::DefaultTraitType(TMeta trait) {
   switch (trait->GetID().GetHash().GetValue()) {
   case Traits::Time::Switch:
      return MetaData::Of<real>();
   case Traits::Resolution::Switch:
   case Traits::MousePosition::Switch:
   case Traits::MouseScroll::Switch:
      return MetaData::Of<vec2>();
   case Traits::ViewTransform::Switch:
   case Traits::ViewTransformInverted::Switch:
   case Traits::ViewProjectTransform::Switch:
   case Traits::ViewProjectTransformInverted::Switch:
   case Traits::ProjectTransform::Switch:
   case Traits::ProjectTransformInverted::Switch:
   case Traits::ModelTransform::Switch:
      return MetaData::Of<mat4>();
   case Traits::Texture::Switch:
      return MetaData::Of<ATexture>();
   default:
      throw Except::Content(pcLogFuncError
         << "Undefined default data type for trait: " << trait);
   }
}

/// Get a default rate of traits																
///	@param trait - the trait definition													
///	@return the associated rate															
RRate MaterialNode::DefaultTraitRate(TMeta trait) {
   switch (trait->GetID().GetHash().GetValue()) {
   case Traits::Time::Switch:
   case Traits::MousePosition::Switch:
   case Traits::MouseScroll::Switch:
      return RRate::PerTick;
   case Traits::ViewTransform::Switch:
   case Traits::ViewTransformInverted::Switch:
   case Traits::ViewProjectTransform::Switch:
   case Traits::ViewProjectTransformInverted::Switch:
      return RRate::PerLevel;
   case Traits::ProjectTransform::Switch:
   case Traits::ProjectTransformInverted::Switch:
   case Traits::FOV::Switch:
   case Traits::Resolution::Switch:
      return RRate::PerCamera;
   case Traits::Texture::Switch:
      return RRate::PerRenderable;
   case Traits::ModelTransform::Switch:
      return RRate::PerInstance;
   case Traits::Position::Switch:
   case Traits::Sampler::Switch:
   case Traits::Aim::Switch:
   case Traits::Color::Switch:
      return RRate::PerVertex;
   default:
      throw Except::Content(pcLogFuncError
         << "Undefined default rate for trait: " << trait);
   }
}

/// Decay a complex type to a fundamental GLSL type									
///	@param meta - the type to decay														
///	@return the decayed type																
DMeta MaterialNode::DecayToGLSLType(DMeta meta) {
   if (meta->Is<rgba>())
      return MetaData::Of<vec4f>();
   else if (meta->Is<rgb>())
      return MetaData::Of<vec3f>();
   else if (meta->Is<vec4f>())
      return MetaData::Of<vec4f>();
   else if (meta->Is<vec3f>())
      return MetaData::Of<vec3f>();
   else if (meta->Is<vec2f>())
      return MetaData::Of<vec2f>();
   else if (meta->Is<vec4d>())
      return MetaData::Of<vec4d>();
   else if (meta->Is<vec3d>())
      return MetaData::Of<vec3d>();
   else if (meta->Is<vec2d>())
      return MetaData::Of<vec2d>();

   LinkedBase outputBase;

   // Color types																			
   if (meta->GetBase<vec4u8>(0, outputBase) && outputBase.mStaticBase.mMapping)
      return MetaData::Of<vec4f>();
   else if (meta->GetBase<vec3u8>(0, outputBase) && outputBase.mStaticBase.mMapping)
      return MetaData::Of<vec3f>();
   else if (meta->GetBase<vec2u8>(0, outputBase) && outputBase.mStaticBase.mMapping)
      return MetaData::Of<vec2f>();
   else if (meta->GetBase<pcu8>(0, outputBase) && outputBase.mStaticBase.mMapping)
      return MetaData::Of<pcr32>();

   // Single precision																	
   if (meta->GetBase<vec4f>(0, outputBase) && outputBase.mStaticBase.mMapping)
      return outputBase.mBase;
   else if (meta->GetBase<vec3f>(0, outputBase) && outputBase.mStaticBase.mMapping)
      return outputBase.mBase;
   else if (meta->GetBase<vec2f>(0, outputBase) && outputBase.mStaticBase.mMapping)
      return outputBase.mBase;
   else if (meta->GetBase<pcr32>(0, outputBase) && outputBase.mStaticBase.mMapping)
      return outputBase.mBase;

   // Double precision																	
   if (meta->GetBase<vec4d>(0, outputBase) && outputBase.mStaticBase.mMapping)
      return outputBase.mBase;
   else if (meta->GetBase<vec3d>(0, outputBase) && outputBase.mStaticBase.mMapping)
      return outputBase.mBase;
   else if (meta->GetBase<vec2d>(0, outputBase) && outputBase.mStaticBase.mMapping)
      return outputBase.mBase;
   else if (meta->GetBase<pcr64>(0, outputBase) && outputBase.mStaticBase.mMapping)
      return outputBase.mBase;

   return nullptr;
}