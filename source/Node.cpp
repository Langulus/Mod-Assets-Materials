///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Node.hpp"
#include "Material.hpp"
#include "MaterialLibrary.hpp"
#include <Flow/Verbs/Create.hpp>
#include <Flow/Verbs/Select.hpp>


/// Material node construction for Nodes::Root                                
///   @param classid - the node type                                          
///   @param material - the parent material                                   
///   @param descriptor - the node descriptor                                 
Node::Node(DMeta classid, Material* material, const Descriptor& descriptor)
   : Unit {classid, descriptor}
   , mDescriptor {descriptor}
   , mMaterial {material} {
   // Satisfy the rest of the descriptor                                
   InnerCreate();
}

/// Material node construction used in the rest of the Nodes                  
///   @param classid - the node type                                          
///   @param descriptor - the node descriptor                                 
Node::Node(DMeta classid, const Descriptor& descriptor)
   : Unit {classid, descriptor}
   , mDescriptor {descriptor} {
   // Account for any Traits::Parent that is Node                       
   descriptor.ForEachDeep(
      [this](const Trait& trait) {
         if (trait.TraitIs<Traits::Parent>()) {
            trait.ForEach([this](const Node* owner) {
               mParent = owner;
               mMaterial = owner->GetMaterial();
            });
         }
      }
   );

   // Satisfy the rest of the descriptor                                
   InnerCreate();
}

/// Parse the normalized descriptor and create subnodes, apply traits         
void Node::InnerCreate() {
   // Save the rate                                                     
   auto rates = mDescriptor.GetTraits<Traits::Rate>();
   if (rates)
      mRate = rates->Last().AsCast<Rate>();

   if (mRate == Rate::Auto)
      mRate = GetMaterial()->GetDefaultRate();

   // Create all sub constructs                                         
   for (auto pair : mDescriptor.mConstructs) {
      if (!pair.mKey->template CastsTo<Node>() &&
          !pair.mKey->template CastsTo<A::Geometry>() &&
          !pair.mKey->template CastsTo<A::Texture>() &&
          !pair.mKey->template CastsTo<A::File>()) {
         Logger::Warning(Self(), "Ignored constructs of type ", pair.mKey);
         continue;
      }

      for (auto& construct : pair.mValue)
         NodeFromConstruct(construct);
   }
}

/// Create a child node from a construct                                      
///   @param construct - the construct to satisfy                             
void Node::NodeFromConstruct(const Construct& construct) {
   if (construct.CastsTo<Node>()) {
      // Create a child node                                            
      Construct local {construct};
      local << Traits::Parent {this};

      auto newInstance = Any::FromMeta(construct.GetType());
      newInstance.Emplace(construct.GetArgument());
      mChildren << newInstance.As<Node*>();
   }
   else if (construct.CastsTo<A::Geometry>()) {
      // Wrap geometries in a Nodes::Scene                              
      TODO();
   }
   else if (construct.CastsTo<A::Texture>()) {
      // Wrap textures in a Nodes::Texture                              
      TODO();
   }
   else if (construct.CastsTo<A::File>()) {
      // Check file format and incorporate compatible data as nodes     
      TODO();
   }
   else Logger::Warning(Self(), "Ignored construct: ", construct);
}

/// Get material library                                                      
///   @return a pointer to the manager                                        
MaterialLibrary* Node::GetLibrary() const noexcept {
   return static_cast<MaterialLibrary*>(mMaterial->GetProducer());
}

/// Get node's environment hierarchy                                          
///   @return the material owners                                             
const Hierarchy& Node::GetOwners() const noexcept {
   return mMaterial->GetOwners();
}

/// Create new nodes                                                          
///   @param verb - the selection verb                                        
void Node::Create(Verb& verb) {
   verb.ForEachDeep([&](const Block& group) {
      group.ForEach([&](const Construct& content) {
         VERBOSE_NODE("Adding scene node: ", content);
         auto scene = EmplaceChildUnique(Nodes::Scene(this, verb));
         verb << scene;
      });
   });
}

/// Interface inputs/outputs in node hierarchy                                
///   @param verb - the selection verb                                        
void Node::Select(Verb& verb) {
   const auto rate = Node::DetermineRate(verb, mRate);

   verb.ForEachDeep([&](const Block& group) {
      group.ForEach(
         [&](const TraitID& trait) {
            // Search for an output from parents, scanning the          
            // hierarchy. If no such symbol was found, a new node       
            // will be created                                          
            auto foundOrCreated = GetValue(
               trait.GetMeta(), nullptr, rate, true);
            verb << foundOrCreated;
         },
         [&](const Trait& trait) {
            // Search for an output from parents, scanning the          
            // hierarchy. If no such symbol was found, a new node       
            // will be created                                          
            auto foundOrCreated = GetValue(
               trait.GetTraitMeta(), trait.GetMeta(), rate, true);
            verb << foundOrCreated;
         }
      );
   });
}

/// Calculate and check rate from a verb frequency                            
///   @param verb - the verb to analyze                                       
///   @param fallback - rate that will be used if verb is PerAuto             
///   @return the refresh rate                                                
RRate Node::DetermineRate(const Verb& verb, RRate fallback) {
   const RRate::Enum fromVerb = static_cast<RRate::Enum>(verb.GetFrequency());
   switch (fromVerb) {
   case Rate::Auto:
      return fallback;
   case PerPixel:
   case PerVertex:
      return fromVerb;
   default:
      if (fromVerb < PerVertex) {
         // Projection provided as a uniform (or constant)              
         return fromVerb;
      }

      // Error condition                                                
      LANGULUS_THROW(Content, "Unsupported rate");
   }
}

/// Get stage from node rate                                                  
///   @return the shader stage that will be used                              
ShaderStage::Enum Node::GetStage() const {
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
void Node::Descend() {
   for (auto child : mChildren)
      child->Generate();
}

/// Commit a code snippet to a specific stage and place                       
///   @param place - the shader token to commit changes at                    
///   @param addition - the code to commit                                    
void Node::Commit(ShaderToken::Enum place, const GLSL& addition) {
   GetProducer()->Commit(GetStage(), place, addition);
}

/// Log the material node hierarchy                                           
void Node::Dump() const {
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

Debug Node::DebugBegin() const {
   Flow::Code result;
   result += ClassMeta()->GetToken();
   result += GASM::Frequency;
   result += mRate;
   result += GASM::OpenScope;
   return result;
}

Debug Node::DebugEnd() const {
   Flow::Code result;
   result += GASM::CloseScope;
   return result;
}

/// For logging                                                               
Node::operator Debug() const {
   Flow::Code result;
   result += DebugBegin();
   result += DebugEnd();
   return result;
}

/// Convert node's output to GLSL code                                        
///   @return GLSL code equivalent to the node's output                       
Node::operator GLSL() const {
   return mOutputs.GetValue(0);
}

/// Generate GLSL code from a construct                                       
///   @param construct - the construct to interpret as GLSL                   
///   @return the GLSL code                                                   
GLSL Node::CodeFromConstruct(const Construct& construct) {
   // Attempt static construction                                       
   Any created;
   try {
      construct.StaticCreation(created);
      return GLSL {created};
   }
   catch (const Except::BadConstruction&) { }

   // Propagate the construct as GLSL                                   
   Any context {mParent};
   auto code = GLSL::Type(construct.GetMeta()) + "(";
   bool separator = false;

   construct.ForEachDeep([&](const Block& group) {
      if (separator)
         code += ", ";

      if (group.Is<Verb>()) {
         // Execute verbs in parent scope if any                        
         Any scopeResults;
         Any scope {group};
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
///   @param scope - the scope to interpret as GLSL                           
///   @return the GLSL code                                                   
GLSL Node::CodeFromScope(const Any& scope) {
   GLSL code;
   Any scopeLocal {scope};
   Any scopeResults;
   Any context {mParent};
   Verb::ExecuteScope(context, scopeLocal, scopeResults);
   scopeResults.ForEach(
      [&code](const MaterialNode& node) {
         code += node.GetOutputSymbol();
      }
   );
   return code;
}

/// Check if a node already exists in parent chain                            
///   @param what - what node to search for                                   
///   @return true if any of the parents match the search                     
bool Node::IsInHierarchy(MaterialNode* what) const {
   return mParent && (mParent == what || mParent->IsInHierarchy(what));
}

/// Add child, also sets parents for child                                    
///   @param child - child pointer to add                                     
void Node::AddChild(MaterialNode* child) {
   if (this == child)
      return;
   if (mChildren.Find(child) != uiNone)
      return;
   if (IsInHierarchy(child))
      return;

   child->mParent = this;
   mChildren << child;
   VERBOSE_NODE("Added child ", Logger::Cyan, child);
}

/// Remove child, also removes parent of child                                
///   @param child - child pointer to remove                                  
void Node::RemoveChild(MaterialNode* child) {
   auto found = mChildren.Find(child);
   if (found.IsSpecial())
      return;

   child->mParent.Reset();
   mChildren.RemoveIndex(static_cast<pcptr>(found));
   VERBOSE_NODE("Removed child ", Logger::Cyan, child);
}

/// Return the first output trait                                             
///   @return trait                                                           
const Trait& Node::GetOutputTrait() const {
   if (mOutputs.IsEmpty())
      LANGULUS_THROW(Content, "No default output available");

   return mOutputs.GetKey(0);
}

/// Search and return an output trait, if any                                 
///   @param trait - the trait to search for in outputs                       
///   @return the symbol or an empty container if nothing was found           
Trait Node::GetOutputTrait(const Trait& trait) const {
   for (pcptr i = 0; i < mOutputs.GetCount(); ++i) {
      auto& key = mOutputs.GetKey(i);
      if (trait.IsSimilar(key))
         return key;
   }

   return {};
}

/// Search and return an output symbol, if any                                
///   @param trait - the trait to search for in outputs                       
///   @return the symbol or an empty container if nothing was found           
GLSL Node::GetOutputSymbol(const Trait& trait) const {
   for (pcptr i = 0; i < mOutputs.GetCount(); ++i) {
      if (trait.IsSimilar(mOutputs.GetKey(i)))
         return mOutputs.GetValue(i);
   }

   return {};
}

/// Return the first output symbol                                            
///   @return the symbol                                                      
const GLSL& MaterialNode::GetOutputSymbol() const {
   if (mOutputs.IsEmpty())
      LANGULUS_THROW(Content, "Node has no outputs");

   return mOutputs.GetValue(0);
}

/// Convert a symbol from one type to another                                 
///   @param trait - the trait to convert                                     
///   @param symbol - the symbol for the original trait                       
///   @param as - the type to convert to                                      
///   @param filler - number for filling empty stuff                          
///   @return the new symbol                                                  
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

   Logger::Error("Can't reinterpret GLSL type ", trait, " to ", as->GetToken());
   LANGULUS_THROW(Content, "Can't reinterpret GLSL type");
}

/// Return the first output symbol, converted as the desired type             
///   @param as - the type we want to convert to                              
///   @param filler - value to fill expanded parts (if any)                   
///   @return the symbol                                                      
GLSL Node::GetOutputSymbolAs(DMeta as, real filler) const {
   if (mOutputs.IsEmpty())
      LANGULUS_THROW(Content, "Node has no outputs");

   return ConvertSymbol(mOutputs.GetKey(0), mOutputs.GetValue(0), as, filler);
}

/// Return the matching output symbol, converted as the desired type          
///   @param trait - the trait we're searching for                            
///   @param as - the type we want to convert to                              
///   @param filler - value to fill expanded parts (if any)                   
///   @return the symbol                                                      
GLSL Node::GetOutputSymbolAs(const Trait& trait, DMeta as, real filler) const {
   for (pcptr i = 0; i < mOutputs.GetCount(); ++i) {
      if (trait.IsSimilar(mOutputs.GetKey(i)))
         return ConvertSymbol(mOutputs.GetKey(i), mOutputs.GetValue(i), as, filler);
   }

   return {};
}

/// Get (or make) node that contains an input we're searching for             
///   @param trait - the trait to search for (nullptr for any)                
///   @param rate - the rate of the trait to use                              
///   @param checkHere - whether or not to check local outputs                
///   @return the symbol and usage                                            
bool Node::InnerGetValue(const Trait& trait, RRate rate, bool checkHere, MaterialNodeValue& output) const {
   // Check here                                                        
   if (checkHere) {
      VERBOSE_NODE("Searching for ", trait, " inside ", *this);
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
         VERBOSE_NODE("Searching for ", trait, " inside ", *child);
         for (auto& key : child->mOutputs.Keys()) {
            if (!trait.IsSimilar(key))
               continue;

            output.BindTo(key, child);
            return true;
         }
      }

      // Then check the parent itself                                   
      VERBOSE_NODE("Searching for ", trait, " inside ", *mParent);
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
///   @param tmeta - the trait to search for (nullptr for any)                
///   @param dmeta - the data to search for (nullptr for any)                 
///   @param rate - the rate of the trait to use                              
///   @param addIfMissing - whether or not to generate default usage          
///   @return the symbol and usage                                            
Nodes::Value Node::GetValue(TMeta tmeta, DMeta dmeta, RRate rate, bool addIfMissing) {
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
      Langulus::Warning(
         "No input available, so adding default one: ", 
         GetProducer()->GenerateInputName(rate, trait)
      );
      result.BindTo(trait, nullptr);
   }

   return result;
}

/// Get input symbol from nodes                                               
///   @param tmeta - the trait to search for (nullptr for any)                
///   @param dmeta - the data to search for (nullptr for any)                 
///   @param rate - the rate of the trait to use                              
///   @param addIfMissing - whether or not to generate default usage          
///   @return the symbol and usage                                            
GLSL Node::GetSymbol(TMeta tmeta, DMeta dmeta, RRate rate, bool addIfMissing) {
   auto found = GetValue(tmeta, dmeta, rate, addIfMissing);
   const auto trait = Trait::FromMeta(tmeta, dmeta);
   auto symbol = found.GetOutputSymbol(trait);
   if (symbol.IsEmpty()) {
      VERBOSE_NODE(Logger::Red, 
         "Undefined input ", tmeta->GetToken(), 
         " (of type ", (dmeta ? dmeta->GetToken() : LiteralText {"any"}), 
         ") @ ", rate
      );
      return {};
   }

   return symbol;
}

/// Get a default type of traits                                              
///   @param trait - the trait definition                                     
///   @return the associated data type                                        
DMeta Node::DefaultTraitType(TMeta trait) {
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
      Langulus::Error("Undefined default data type for trait: ", trait);
      LANGULUS_THROW(Content, "Undefined default data type for trait");
   }
}

/// Get a default rate of traits                                              
///   @param trait - the trait definition                                     
///   @return the associated rate                                             
RRate Node::DefaultTraitRate(TMeta trait) {
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
      Langulus::Error("Undefined default rate for trait: ", trait);
      LANGULUS_THROW(Content, "Undefined default rate for trait");
   }
}

/// Decay a complex type to a fundamental GLSL type                           
///   @param meta - the type to decay                                         
///   @return the decayed type                                                
DMeta Node::DecayToGLSLType(DMeta meta) {
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