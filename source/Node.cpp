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
#include "nodes/Value.hpp"


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

/// Material node construction for members/locals                             
///   @param classid - the node type                                          
///   @param parent - the parent node                                         
///   @param descriptor - the node descriptor                                 
Node::Node(DMeta classid, Node* parent, const Descriptor& descriptor)
   : Unit {classid, descriptor}
   , mDescriptor {descriptor}
   , mParent {parent}
   , mMaterial {parent->GetMaterial()} {
   parent->mChildren << this;
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
            return !trait.ForEach([this](const Node* owner) {
               mParent = owner;
               const_cast<Node*>(owner)->mChildren << this;
               mMaterial = owner->GetMaterial();
               return Flow::Break;
            });
         }
         return Flow::Continue;
      }
   );

   // Satisfy the rest of the descriptor                                
   InnerCreate();
}

/// Parse the normalized descriptor and create subnodes, apply traits         
/// This is a common parser used in all nodes' constructors                   
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
Node* Node::NodeFromConstruct(const Construct& construct) {
   if (construct.CastsTo<Node>()) {
      // Create a child node                                            
      VERBOSE_NODE("Adding node: ", construct);
      Construct local {construct};
      local << Traits::Parent {this};

      auto newInstance = Any::FromMeta(construct.GetType());
      newInstance.Emplace(construct.GetArgument());
      mChildren << newInstance.As<Node*>();
      return mChildren.Last();
   }
   else if (construct.CastsTo<A::Geometry>()) {
      // Wrap geometries in a Nodes::Scene                              
      VERBOSE_NODE("Adding scene node: ", construct);
      TODO();
   }
   else if (construct.CastsTo<A::Texture>()) {
      // Wrap textures in a Nodes::Texture                              
      VERBOSE_NODE("Adding texture node: ", construct);
      TODO();
   }
   else if (construct.CastsTo<A::File>()) {
      // Check file format and incorporate compatible data as nodes     
      VERBOSE_NODE("Adding node from file: ", construct);
      TODO();
   }
   else Logger::Warning(Self(), "Ignored construct: ", construct);
   return {};
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
      group.ForEach(
         [&](const MetaData& type) {
            verb << NodeFromConstruct(Construct {&type});
         },
         [&](const Construct& content) {
            verb << NodeFromConstruct(content);
         }
      );
   });
}

/// Interface inputs/outputs in node hierarchy                                
///   @param verb - the selection verb                                        
void Node::Select(Verb& verb) {
   Rate rate = Rate::Auto;
   verb.ForEachDeep([&](const Block& group) {
      group.ForEach(
         [&](const Rate& r) {
            rate = r;
         },
         [&](const MetaTrait& trait) {
            auto foundOrCreated = GetValue(&trait, nullptr, rate, true);
            verb << foundOrCreated;
         },
         [&](const Trait& trait) {
            auto foundOrCreated = GetValue(trait.GetTrait(), trait.GetType(), rate, true);
            verb << foundOrCreated;
         }
      );
   });
}

/// Get stage from node rate                                                  
///   @return the shader stage that will be used                              
Offset Node::GetStage() const {
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
/// Also mark this node as generated                                          
void Node::Descend() {
   mGenerated = true;
   VERBOSE_NODE("Generating code...");
   for (auto child : mChildren)
      child->Generate();
}

/// Commit a code snippet to a specific stage and place                       
///   @param place - the shader token to commit changes at                    
///   @param addition - the code to commit                                    
void Node::Commit(const Token& place, const GLSL& addition) {
   mMaterial->Commit(GetStage(), place, addition);
}

/// Log the material node hierarchy                                           
void Node::Dump() const {
   if (mChildren.IsEmpty()) {
      Logger::Verbose(Self(), " {}");
      return;
   }

   const auto scope = Logger::Verbose(Self(), " {", Logger::Tabs {});
   for (auto child : mChildren)
      child->Dump();
   Logger::Verbose('}');
}

/// Opens node debugging scope                                                
///   @return the openning string                                             
Debug Node::DebugBegin() const {
   Code result;
   result += GetToken();
   result += Code::OpenScope;
   result += Serialize<Code>(Traits::Rate {mRate});
   return result;
}

/// Closes node debugging scope                                               
///   @return the closing string                                              
Debug Node::DebugEnd() const {
   Code result;
   result += Code::CloseScope;
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
/*GLSL Node::CodeFromConstruct(const Construct& construct) {
   // Attempt static construction                                       
   Any created;
   try {
      construct.StaticCreation(created);
      return GLSL {created};
   }
   catch (const Except::Construct&) { }

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
      [&code](const Node& node) {
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
}*/

/// Return the first output trait                                             
///   @return trait                                                           
const Trait& Node::GetOutputTrait() const {
   LANGULUS_ASSERT(!mOutputs.IsEmpty(), Material, "Node has no outputs");
   return mOutputs.GetKey(0);
}

/// Search and return an output trait, if any                                 
///   @param trait - the trait to search for in outputs                       
///   @return the symbol or an empty container if nothing was found           
Trait Node::GetOutputTrait(const Trait& trait) const {
   for (auto pair : mOutputs) {
      if (trait.IsSimilar(pair.mKey))
         return pair.mKey;
   }
   return {};
}

/// Search and return an output symbol, if any                                
///   @param trait - the trait to search for in outputs                       
///   @return the symbol or an empty container if nothing was found           
GLSL Node::GetOutputSymbol(const Trait& trait) const {
   for (auto pair : mOutputs) {
      if (trait.IsSimilar(pair.mKey))
         return pair.mValue;
   }
   return {};
}

/// Return the first output symbol                                            
///   @return the symbol                                                      
const GLSL& Node::GetOutputSymbol() const {
   LANGULUS_ASSERT(!mOutputs.IsEmpty(), Material, "Node has no outputs");
   return mOutputs.GetValue(0);
}

/// Convert a symbol from one type to another in GLSL                         
///   @param trait - the trait to convert                                     
///   @param symbol - the symbol for the original trait                       
///   @param as - the type to convert to                                      
///   @param filler - number for filling empty stuff                          
///   @return the new symbol                                                  
GLSL ConvertSymbol(const Trait& trait, const GLSL& symbol, DMeta as, Real filler) {
   auto from = trait.GetType();
   if (from->CastsTo(as))
      return symbol;
   
   using AM4 = A::MatrixOfSize<4>;
   using AM3 = A::MatrixOfSize<3>;
   using AM2 = A::MatrixOfSize<2>;

   using AV4 = A::VectorOfSize<4>;
   using AV3 = A::VectorOfSize<3>;
   using AV2 = A::VectorOfSize<2>;
   using AV1 = A::VectorOfSize<1>;

   constexpr Token TypeX = "{0}({1})";
   constexpr Token MatXtoMatY = "{0}({1}[0], 0.0, {1}[1], 0.0, {1}[2], 0.0, {1}[3], {2})";
   constexpr Token Mat2toMat4 = "mat4({1}[0], 0.0, 0.0, {1}[1], 0.0, 0.0, {1}[2], 0.0, 0.0, {1}[3], 0.0, {2})";

   // Matrix types                                                      
   if (from->template CastsTo<AM4>()) {
      if (as->template CastsTo<AM3>())
         return TemplateFill(TypeX, "mat3", symbol);
      else if (as->template CastsTo<AM2>())
         return TemplateFill(TypeX, "mat2", symbol);
   }
   else if (from->template CastsTo<AM3>()) {
      if (as->template CastsTo<AM4>())
         return TemplateFill(MatXtoMatY, "mat4", symbol, filler);
      else if (as->template CastsTo<AM2>())
         return TemplateFill(TypeX, "mat2", symbol);
   }
   else if (from->template CastsTo<AM2>()) {
      if (as->template CastsTo<AM4>())
         return TemplateFill(Mat2toMat4, symbol, filler);
      else if (as->template CastsTo<AM3>())
         return TemplateFill(MatXtoMatY, "mat3", symbol, filler);
   }
   // Vector types                                                      
   else if (from->template CastsTo<AV4>()) {
      if (as->template CastsTo<AV4>())
         return symbol;
      else if (as->template CastsTo<AV3>())
         return symbol + ".xyz";
      else if (as->template CastsTo<AV2>())
         return symbol + ".xy";
      else if (as->template CastsTo<AV1>() || as->template CastsTo<A::Number>())
         return symbol + ".x";
   }
   else if (from->template CastsTo<AV3>()) {
      if (as->template CastsTo<AV4>())
         return "vec4(" + symbol + ", 1.0)";
      else if (as->template CastsTo<AV3>())
         return symbol;
      else if (as->template CastsTo<AV2>())
         return symbol + ".xy";
      else if (as->template CastsTo<AV1>() || as->template CastsTo<A::Number>())
         return symbol + ".x";
   }
   else if (from->template CastsTo<AV2>()) {
      if (as->template CastsTo<AV4>())
         return "vec4(" + symbol + ", " + filler + ", 1.0)";
      else if (as->template CastsTo<AV3>())
         return "vec3(" + symbol + ", " + filler + ")";
      else if (as->template CastsTo<AV2>())
         return symbol;
      else if (as->template CastsTo<AV1>() || as->template CastsTo<A::Number>())
         return symbol + ".x";
   }
   else if (from->template CastsTo<AV1>() || from->template CastsTo<A::Number>()) {
      if (as->template CastsTo<AV4>())
         return "vec4(vec3(" + symbol + "), 1.0)";
      else if (as->template CastsTo<AV3>())
         return TemplateFill(TypeX, "vec3", symbol);
      else if (as->template CastsTo<AV2>())
         return TemplateFill(TypeX, "vec2", symbol);
      else if (as->template CastsTo<AV1>() || as->template CastsTo<A::Number>())
         return symbol;
   }

   Logger::Error("Can't convert trait ", trait, " to ", as);
   LANGULUS_THROW(Material, "Can't convert type");
}

/// Return the first output symbol, converted as the desired type             
///   @param as - the type we want to convert to                              
///   @param filler - value to fill expanded parts (if any)                   
///   @return the symbol                                                      
GLSL Node::GetOutputSymbolAs(DMeta as, Real filler) const {
   LANGULUS_ASSERT(!mOutputs.IsEmpty(), Material, "Node has no outputs");
   for (auto pair : mOutputs)
      return ConvertSymbol(pair.mKey, pair.mValue, as, filler);
}

/// Return the matching output symbol, converted as the desired type          
///   @param trait - the trait we're searching for                            
///   @param as - the type we want to convert to                              
///   @param filler - value to fill expanded parts (if any)                   
///   @return the symbol                                                      
GLSL Node::GetOutputSymbolAs(const Trait& trait, DMeta as, Real filler) const {
   for (auto pair : mOutputs) {
      if (trait.IsSimilar(pair.mKey))
         return ConvertSymbol(pair.mKey, pair.mValue, as, filler);
   }

   return {};
}

/// Get (or make) node that contains an input we're searching for             
///   @param trait - the trait to search for (nullptr for any)                
///   @param rate - the rate of the trait to use                              
///   @param checkHere - whether or not to check local outputs                
///   @param output - [out] the resulting value node                          
///   @return true if output has been set                                     
bool Node::InnerGetValue(const Trait& trait, Rate rate, bool checkHere, Nodes::Value& output) const {
   // Check here                                                        
   if (checkHere) {
      VERBOSE_NODE("Searching for ", trait, " inside ", this);
      for (auto pair : mOutputs) {
         if (!trait.IsSimilar(pair.mKey))
            continue;

         output.BindTo(pair.mKey, this);
         return true;
      }
   }

   if (mParent) {
      // Scan previous node's outputs                                   
      const auto thisIndex = mParent->mChildren.Find(this);
      const Offset start = thisIndex == IndexNone
         ? mParent->mChildren.GetCount()
         : thisIndex.GetOffset();

      for (Offset i = start; i > 0; --i) {
         // Check outputs of all children up to this one                
         auto child = mParent->mChildren[i - 1];
         VERBOSE_NODE("Searching for ", trait, " inside ", *child);
         for (auto pair : child->mOutputs) {
            if (!trait.IsSimilar(pair.mKey))
               continue;

            output.BindTo(pair.mKey, child);
            return true;
         }
      }

      // Then check the parent itself                                   
      VERBOSE_NODE("Searching for ", trait, " inside ", mParent);
      for (auto pair : mParent->mOutputs) {
         if (!trait.IsSimilar(pair.mKey))
            continue;

         output.BindTo(pair.mKey, mParent);
         return true;
      }

      // Climb the hierarchy in search of more                          
      for (Offset i = start; i > 0; --i) {
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
Nodes::Value Node::GetValue(TMeta tmeta, DMeta dmeta, Rate rate, bool addIfMissing) {
   if (rate == Rate::Auto)
      rate = mRate;

   const auto trait = Trait::FromMeta(tmeta, dmeta);
   auto result = Nodes::Value::Local(this, trait, rate);
   if (InnerGetValue(trait, rate, true, result)) {
      if (result.GetRate() < rate) {
         // We have to build a bridge to this rate                      
         mMaterial->AddOutput(result.GetRate(), result.GetOutputTrait(), false);
         auto newName = mMaterial->AddInput(rate, result.GetOutputTrait(), false);
         result.mRate = rate;
         result.mOutputs.Values()[0] = newName;
      }

      return result;
   }

   // Undefined symbol                                                  
   if (addIfMissing) {
      Logger::Warning(
         "No input available, so adding default one: ", 
         GetMaterial()->GenerateInputName(rate, trait)
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
GLSL Node::GetSymbol(TMeta tmeta, DMeta dmeta, Rate rate, bool addIfMissing) {
   auto found = GetValue(tmeta, dmeta, rate, addIfMissing);
   const auto trait = Trait::FromMeta(tmeta, dmeta);
   auto symbol = found.GetOutputSymbol(trait);
   if (symbol.IsEmpty()) {
      VERBOSE_NODE(Logger::Red, 
         "Undefined input ", tmeta, 
         " (of type ", (dmeta ? dmeta->mToken : "any"), 
         ") @ ", rate
      );
      return {};
   }

   return symbol;
}

/// Get a default type of each of the standard traits                         
///   @param trait - the trait definition                                     
///   @return the associated data type                                        
const TraitProperties& Node::GetDefaultTrait(TMeta trait) {
   static TUnorderedMap<TMeta, TraitProperties> properties;
   if (properties.IsEmpty()) {
      properties[MetaOf<Traits::Time>()] =
         {MetaOf<Real>(), PerTick};
      properties[MetaOf<Traits::MousePosition>()] =
         {MetaOf<Vec2>(), PerTick};
      properties[MetaOf<Traits::MouseScroll>()] =
         {MetaOf<Vec2>(), PerTick};

      properties[MetaOf<Traits::Size>()] =
         {MetaOf<Vec2>(), PerCamera};
      properties[MetaOf<Traits::Projection>()] =
         {MetaOf<Mat4>(), PerCamera};
      properties[MetaOf<Traits::FOV>()] =
         {MetaOf<Real>(), PerCamera};

      properties[MetaOf<Traits::View>()] =
         {MetaOf<Mat4>(), PerLevel};

      properties[MetaOf<Traits::Texture>()] =
         {MetaOf<A::Texture>(), PerRenderable};

      properties[MetaOf<Traits::Transform>()] =
         {MetaOf<Mat4>(), PerInstance};

      properties[MetaOf<Traits::Place>()] =
         {MetaOf<Vec3>(), PerVertex};
      properties[MetaOf<Traits::Sampler>()] =
         {MetaOf<Vec2>(), PerVertex};
      properties[MetaOf<Traits::Aim>()] =
         {MetaOf<Vec3>(), PerVertex};
      properties[MetaOf<Traits::Color>()] =
         {MetaOf<Vec4>(), PerVertex};
   }

   auto found = properties.FindKey(trait);
   if (found)
      return properties.GetKey(found);

   Langulus::Error("Undefined trait: ", trait);
   LANGULUS_THROW(Material, "Undefined trait");
}

/// Decay a complex type to a fundamental GLSL type                           
///   @param meta - the type to decay                                         
///   @return the decayed type                                                
DMeta Node::DecayToGLSLType(DMeta meta) {
   if (meta->template CastsTo<Double>(4))
      return MetaData::Of<Vec4d>();
   else if (meta->template CastsTo<Double>(3))
      return MetaData::Of<Vec3d>();
   else if (meta->template CastsTo<Double>(2))
      return MetaData::Of<Vec2d>();
   else if (meta->template CastsTo<Double>(1))
      return MetaData::Of<Double>();

   else if (meta->template CastsTo<Float>(4) || meta->template CastsTo<A::Number>(4))
      return MetaData::Of<Vec4f>();
   else if (meta->template CastsTo<Float>(3) || meta->template CastsTo<A::Number>(3))
      return MetaData::Of<Vec3f>();
   else if (meta->template CastsTo<Float>(2) || meta->template CastsTo<A::Number>(2))
      return MetaData::Of<Vec2f>();
   else if (meta->template CastsTo<Float>(1) || meta->template CastsTo<A::Number>(1))
      return MetaData::Of<Float>();
   else
      LANGULUS_THROW(Material, "Can't decay type");
}