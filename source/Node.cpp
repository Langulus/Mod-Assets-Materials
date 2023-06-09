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
#include "nodes/Value.hpp"
#include <Math/Randomness/SimplexNoise.hpp>
#include <Math/Verbs.hpp>


/// Material node construction for Nodes::Root                                
///   @param classid - the node type                                          
///   @param material - the parent material                                   
///   @param descriptor - the node descriptor                                 
Node::Node(DMeta classid, Material* material, const Descriptor& descriptor)
   : Unit {classid, {}} // The descriptor might contain Thing's owner,  
                        // so we don't forward it to the unit           
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
   : Unit {classid, {}} // The descriptor might contain Thing's owner,  
                        // so we don't forward it to the unit           
   , mDescriptor {descriptor}
   , mMaterial {parent->GetMaterial()}
   , mParent {parent} {
   parent->mChildren << this;
   // Satisfy the rest of the descriptor                                
   InnerCreate();
}

/// Material node construction used in the rest of the Nodes                  
///   @param classid - the node type                                          
///   @param descriptor - the node descriptor                                 
Node::Node(DMeta classid, const Descriptor& descriptor)
   : Unit {classid, {}} // The descriptor might contain Thing's owner,  
                        // but we don't forward it to the unit          
   , mDescriptor {descriptor} {
   // Account for any Traits::Parent that is Node                       
   // We handle those here, because they get omitted on normalization   
   descriptor.ForEachDeep(
      [this](const Trait& trait) {
         if (trait.TraitIs<Traits::Parent>()) {
            return !trait.ForEach([this](const Node* owner) {
               auto mutableOwner = const_cast<Node*>(owner);
               mParent = mutableOwner;
               mutableOwner->mChildren << this;
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
   const auto scope = Logger::Verbose(Self(),
      "Creating node...", Logger::Tabs {});

   // Save the rate                                                     
   mRate = GetMaterial()->GetDefaultRate();
   auto rates = mDescriptor.GetTraits<Traits::Rate>();
   if (rates)
      mRate = rates->Last().AsCast<Rate>();

   // Create all sub constructs                                         
   for (const auto pair : mDescriptor.mConstructs) {
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
   
   // Consider all provided data                                        
   for (const auto pair : mDescriptor.mAnythingElse) {
      if (pair.mKey->template CastsTo<Code>()) {
         // Execute each code snippet                                   
         Math::RegisterTraits();
         Math::RegisterVerbs();

         for (auto& snippet : pair.mValue)
            Run(snippet.Get<Code>());
      }
      else if (pair.mKey->template CastsTo<Rate>()) {
         // Set rate through data only                                  
         mRate = pair.mValue.Last().Get<Rate>();
      }
      else {
         Logger::Warning(Self(), "Ignored data of type ", pair.mKey);
         continue;
      }
   }
}

/// Create a child node from a construct                                      
///   @param construct - the construct to satisfy                             
///   @return a pointer to the child node                                     
Node* Node::NodeFromConstruct(const Construct& construct) {
   if (construct.CastsTo<Node>()) {
      // Create a child node                                            
      VERBOSE_NODE("Adding node: ", construct);
      Construct local {construct};
      local << Traits::Parent {this};

      auto newInstance = Any::FromMeta(construct.GetType());
      newInstance.Emplace(local.GetArgument());
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

/// Create new nodes                                                          
///   @param verb - the selection verb                                        
void Node::Create(Verb& verb) {
   verb.ForEachDeep([&](const Block& group) {
      group.ForEach(
         [&](DMeta type) {
            verb << NodeFromConstruct(Construct {type});
         },
         [&](const Construct& content) {
            verb << NodeFromConstruct(content);
         }
      );
   });
}

/// Select symbols/nodes inside the node hierarchy                            
/// You can filter based on data/node type, trait, index, and rate            
///   @param verb - the selection verb                                        
void Node::Select(Verb& verb) {
   if (verb.GetMass() <= 0)
      return;

   Rate rateFilter = Rate::Auto;
   Index index = IndexNone;
   TMeta traitFilter = {};
   DMeta dataFilter = {};

   // Collect filters from verb argument                                
   verb.ForEachDeep([&](const Block& group) {
      group.ForEach(
         [&](const Rate& r)      noexcept { rateFilter = r; },
         [&](const Index& i)     noexcept { index = i; },
         [&](const Real& i)      noexcept { index = static_cast<Index>(i); },
         [&](TMeta trait)        noexcept { traitFilter = trait; },
         [&](DMeta type)         noexcept { dataFilter = type; },
         [&](const Trait& trait) noexcept {
            dataFilter  = trait.GetType();
            traitFilter = trait.GetTrait();
         }
      );
   });

   if (rateFilter == Rate::Auto || !index || !traitFilter || !dataFilter)
      return;

   // Search for the symbol                                             
   const auto found = GetSymbol(traitFilter, dataFilter, rateFilter, index);
   if (found)
      verb << found;
}

/// An arithmetic verb implementation                                         
/// Uses a pattern to modify all output symbols                               
///   @param verb - the verb to satisfy                                       
///   @param pos - the positive pattern                                       
///   @param neg - the negative pattern (optional)                            
///   @param unary - the unary pattern (optional)                             
void Node::ArithmeticVerb(Verb& verb, const Token& pos, const Token& neg, const Token& unary) {
   if (verb.GetMass() == 0)
      return;

   const bool inverse = verb.GetMass() < 0;
   bool success {};
   if (verb.IsEmpty() && inverse && unary.size()) {
      // No argument, so an unary minus sign                            
      ForEachOutput([&success,&unary](Symbol& symbol) {
         symbol.mCode = Text::TemplateRt(unary, symbol.mCode);
         success = true;
      });

      if (!success)
         Logger::Warning(Self(), "Unable to ", verb);
      verb << this;
      return;
   }

   // Scan arguments: anything convertible to GLSL can be added to      
   // output symbols' expressions                                       
   verb.ForEachDeep([&](const Block& group) {
      group.ForEachElement([&](const Block& element) {
         try {
            const auto code = element.AsCast<GLSL>();
            if (code.IsEmpty())
               return;

            if (neg.empty() || !inverse) {
               ForEachOutput([&](Symbol& symbol) {
                  symbol.mCode = Text::TemplateRt(pos, symbol.mCode, code);
                  success = true;
               });
            }
            else {
               ForEachOutput([&](Symbol& symbol) {
                  symbol.mCode = Text::TemplateRt(neg, symbol.mCode, code);
                  success = true;
               });
            }
         }
         catch (...) { }
      });
   });

   if (!success)
      Logger::Warning(Self(), "Unable to ", verb);
   verb << this;
}

/// Add/subtract inputs                                                       
///   @param verb - the addition/subtraction verb                             
void Node::Add(Verb& verb) {
   ArithmeticVerb(verb, "({} + {})", "({} - {})", "-{}");
}

/// Multiply/divide inputs                                                    
///   @param verb - the multiplication verb                                   
void Node::Multiply(Verb& verb) {
   ArithmeticVerb(verb, "({} * {})", "({} / {})", "1.0 / {}");
}

/// Modulate inputs                                                           
///   @param verb - the modulation verb                                       
void Node::Modulate(Verb& verb) {
   ArithmeticVerb(verb, "mod({}, {})");
}

/// Exponentiate inputs                                                       
///   @param verb - the exponentiation verb                                   
void Node::Exponent(Verb& verb) {
   ArithmeticVerb(verb, "pow({}, {})");
}

/// Randomize inputs                                                          
///   @param verb - the randomization verb                                    
void Node::Randomize(Verb& verb) {
   if (verb.GetMass() == 0)
      return;

   // Collect randomization methods and output types                    
   DMeta otype {};
   Text method = "simplex";
   verb.ForEachDeep([&](const Block& group) {
      group.ForEach(
         [&](const Text& token) { method = token; },
         [&](const DMeta& t)    { otype = t; }
      );
   });

   // Randomize each output symbol                                      
   bool success {};
   ForEachOutput([&](Symbol& symbol) {
      DMeta itype = symbol.mTrait.GetType();

      // Call the appropriate noise function                            
      if (otype->template CastsTo<A::Number>(1)) {
         if (itype->template CastsTo<A::Number>(2))
            AddDefine("SimplexNoise1", TSimplex<1, 2, float>::template Hash<true>());
         else if (itype->template CastsTo<A::Number>(3))
            AddDefine("SimplexNoise1", TSimplex<1, 3, float>::template Hash<true>());
         else TODO();

         symbol.mCode = Text::TemplateRt("SimplexNoise1({})", symbol.mCode);
         success = true;
      }
      else TODO();
   });


   if (!success)
      Logger::Warning(Self(), "Unable to ", verb);
   verb << this;
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

/// Add a definition at the node's rate                                       
///   @param name - the definition name (used to detect duplications)         
///   @param code - the code snippet to add                                   
void Node::AddDefine(const Token& name, const GLSL& code) {
   mMaterial->AddDefine(mRate, name, code);
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
         return Text::TemplateRt(TypeX, "mat3", symbol);
      else if (as->template CastsTo<AM2>())
         return Text::TemplateRt(TypeX, "mat2", symbol);
   }
   else if (from->template CastsTo<AM3>()) {
      if (as->template CastsTo<AM4>())
         return Text::TemplateRt(MatXtoMatY, "mat4", symbol, filler);
      else if (as->template CastsTo<AM2>())
         return Text::TemplateRt(TypeX, "mat2", symbol);
   }
   else if (from->template CastsTo<AM2>()) {
      if (as->template CastsTo<AM4>())
         return Text::TemplateRt(Mat2toMat4, symbol, filler);
      else if (as->template CastsTo<AM3>())
         return Text::TemplateRt(MatXtoMatY, "mat3", symbol, filler);
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
         return Text::TemplateRt(TypeX, "vec3", symbol);
      else if (as->template CastsTo<AV2>())
         return Text::TemplateRt(TypeX, "vec2", symbol);
      else if (as->template CastsTo<AV1>() || as->template CastsTo<A::Number>())
         return symbol;
   }

   Logger::Error("Can't convert trait ", trait, " to ", as);
   LANGULUS_THROW(Material, "Can't convert type");
}

/// Get a default type of each of the standard traits                         
///   @param trait - the trait definition                                     
///   @return the default trait properties                                    
const Node::DefaultTrait& Node::GetDefaultTrait(TMeta trait) {
   static TUnorderedMap<TMeta, DefaultTrait> properties;

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

   auto found = properties.FindKeyIndex(trait);
   if (found)
      return properties.GetValue(found);

   Logger::Error("Undefined trait: ", trait);
   LANGULUS_THROW(Material, "Undefined trait");
}

/// Decay a complex type to a fundamental GLSL type                           
///   @param meta - the type to decay                                         
///   @return the decayed type                                                
DMeta Node::DecayToGLSLType(DMeta meta) {
   if (meta->template CastsTo<Double>(4))
      return MetaOf<Vec4d>();
   else if (meta->template CastsTo<Double>(3))
      return MetaOf<Vec3d>();
   else if (meta->template CastsTo<Double>(2))
      return MetaOf<Vec2d>();
   else if (meta->template CastsTo<Double>(1))
      return MetaOf<Double>();

   else if (meta->template CastsTo<Float>(4) || meta->template CastsTo<A::Number>(4))
      return MetaOf<Vec4f>();
   else if (meta->template CastsTo<Float>(3) || meta->template CastsTo<A::Number>(3))
      return MetaOf<Vec3f>();
   else if (meta->template CastsTo<Float>(2) || meta->template CastsTo<A::Number>(2))
      return MetaOf<Vec2f>();
   else if (meta->template CastsTo<Float>(1) || meta->template CastsTo<A::Number>(1))
      return MetaOf<Float>();
   else
      LANGULUS_THROW(Material, "Can't decay type");
}

/// Select a symbol from the node hierarchy                                   
/// Checks local symbols first, then climbs up the hierarchy up to Root, where
/// it starts selecting global material inputs/constants                      
///   @param t - trait type filter                                            
///   @param d - data type filter                                             
///   @param r - rate filter                                                  
///   @param i - index filter                                                 
///   @return a pointer to the symbol, or nullptr if not found                
Symbol* Node::GetSymbol(TMeta t, DMeta d, Rate r, Index i) {
   Offset nth = i.IsSpecial() ? 0 : i.GetOffset();

   if (t) {
      // Filter by traits                                               
      const auto foundt = mLocalsT.FindKeyIndex(t);
      if (foundt) {
         auto& candidates = mLocalsT.GetValue(foundt);
         if (i && i.IsSpecial()) {
            // Pick a special index                                     
            auto& candidate = candidates[i];
            if (candidate.MatchesFilter(d, r))
               return &candidate;
         }
         else {
            // Pick specific match from all candidates                  
            for (auto& candidate : candidates) {
               if (candidate.MatchesFilter(d, r)) {
                  if (0 == nth)
                     return &candidate;
                  --nth;
               }
            }
         }
      }
   }
   else if (d) {
      // Filter by data type                                            
      for (auto pair : mLocalsD) {
         if (!pair.mKey->CastsTo(d))
            continue;

         auto& candidates = pair.mValue;
         if (i && i.IsSpecial()) {
            // Pick a special index                                     
            auto& candidate = candidates[i];
            if (candidate.MatchesFilter(d, r))
               return &candidate;
         }
         else {
            // Pick specific match from all candidates                  
            for (auto& candidate : candidates) {
               if (candidate.MatchesFilter(d, r)) {
                  if (0 == nth)
                     return &candidate;
                  --nth;
               }
            }
         }
      }
   }
   else if (r != Rate::Auto) {
      // Filter by rate                                                 
      TODO();
   }
   else if (i) {
      // Just return the nth (filter by index only)                     
      TODO();
   }

   // Nothing was found                                                 
   return nullptr;
}