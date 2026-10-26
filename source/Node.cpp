///                                                                           
/// Langulus::Module::Art::Materials                                          
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
///                                                                           
#include "Node.hpp"
#include "Material.hpp"
#include "MaterialLibrary.hpp"
#include "nodes/Value.hpp"
#include <Langulus/Randomness/SimplexNoise.hpp>
#include <Langulus/CppAPI/Graphics.hpp>
#include <Langulus/CppAPI/Platform.hpp>


/// Material node construction for Nodes::Root                                
///   @param material the parent material                                     
///   @param descriptor the node descriptor                                   
Node::Node(Material* material, Many const& descriptor)
   : Node {descriptor} {
   mMaterial = material;
   mDescriptor = descriptor;
}

/// Material node construction for members/locals                             
///   @param parent the parent node                                           
///   @param descriptor the node descriptor                                   
Node::Node(Node* parent, Many const& descriptor)
   : Downcastable {this}
   , mDescriptor  {descriptor} {
   // Add the Node to the hierarchy                                     
   if (parent)
      parent->AddChild(this);
}

/// Material node construction used in the rest of the Nodes                  
///   @param descriptor the node descriptor                                   
Node::Node(Many const& descriptor)
   : Downcastable {this}
   , mDescriptor  {descriptor} {
   // Add the Node to the hierarchy                                     
   Node* owner {};
   if (descriptor.ExtractTag<Tags::Parent>(owner) and owner)
      owner->AddChild(this);
}

/// Detach all nodes from the hierarchy, reducing references                  
void Node::Detach() {
   VERBOSE_NODE_TAB("Destroying (", GetReferences(), " uses):");
   mDescriptor.Reset();

   // Decouple all children from their parent                           
   for (auto& child : mChildren) {
      VERBOSE_NODE("Decoupling child: ", child);
      child->mParent.Reset();
      VERBOSE_NODE("...", GetReferences(), " uses remain");
   }

   for (auto& child : mChildren) {
      child->Detach();
      VERBOSE_NODE("...", GetReferences(), " uses remain");
   }
}

/// Node destructor                                                           
Node::~Node() {
   Detach();
}

/// Parse the normalized descriptor and create subnodes, apply tags           
/// This is a common parser used in all nodes' constructors                   
void Node::InnerCreate() {
   VERBOSE_NODE_TAB("Creating node...");

   // Save the rate                                                     
   mRate = GetMaterial()->GetDefaultRate();
   if (mDescriptor.ExtractTag<Tags::Rate>(mRate)) {
      VERBOSE_NODE("Rate changed (via tag) to: ", mRate);
   }

   // Create all sub recipes                                            
   mDescriptor.ForEachRecipe([&](Recipe const& c) {
      NodeFromConstruct(c);
   });
   
   // Consider all provided data                                        
   mDescriptor.ForEachTail([&](Many const& data) {
      if (data.CastsTo<Flow::Code>()) {
         // Execute code snippet                                        
         auto const& subcode = data.Get<Flow::Code>();
         VERBOSE_NODE_TAB("Executing subcode: ", subcode);
         Run(subcode);
      }
      else if (data.CastsTo<Rate>()) {
         // Set rate through data only                                  
         mRate = data.Get<Rate>();
         VERBOSE_NODE("Rate changed (via data) to: ", mRate);
      }
      else Logger::Warning(this, ": Ignored data: ", data);
   });
}

/// Create a child node from a recipe                                         
///   @param recipe the recipe to satisfy                                     
///   @return a pointer to the child node                                     
auto Node::NodeFromConstruct(Recipe const& recipe) -> Node* {
   if (not recipe.CastsTo<Node>()) {
      Logger::Warning(this, ": Ignored recipe: ", recipe);
      return {};
   }

   // Create a child node                                               
   VERBOSE_NODE_TAB("Adding node: ", recipe);
   Recipe local {recipe};
   local << Tags::Parent {this};

   auto newInstance = Many::Typed(recipe.GetTarget());
   newInstance.Emplace(local.GetDescriptor());
   return newInstance.As<Node*>();
}

/// Get material library                                                      
///   @return a pointer to the manager                                        
auto Node::GetLibrary() const noexcept -> MaterialLibrary* {
   return mMaterial->GetProducer().As<MaterialLibrary>();
}

/// Create new nodes                                                          
///   @param verb the selection verb                                          
void Node::Create(Verb& verb) {
   verb.ForEachDeep([&](Many const& group) {
      group.ForEach(
         [&](DMeta type) {
            verb << NodeFromConstruct(Recipe {type});
         },
         [&](Recipe const& content) {
            verb << NodeFromConstruct(content);
         }
      );
   });
}

/// Select symbols/nodes inside the node hierarchy                            
/// You can filter based on data/node type, tag, index, and rate              
///   @param verb the selection verb                                          
void Node::Select(Verb& verb) {
   if (verb.GetMass() <= 0)
      return;

   RefreshRate rateFilter = Rate::Auto;
   Index::At index = Index::None;
   TMeta traitFilter = {};
   DMeta dataFilter = {};

   // Collect filters from verb argument                                
   verb.ForEachDeep([&](Many const& group) {
      group.ForEach(
         [&](RefreshRate  r) noexcept { rateFilter = r; },
         [&](Index::At    i) noexcept { index = i; },
         [&](Real         i)          { index = static_cast<Index::At>(i); },
         [&](TMeta        t) noexcept { traitFilter = t; },
         [&](DMeta        t) noexcept { dataFilter = t; },
         [&](Tag const& tag) noexcept {
            dataFilter  = tag.GetType();
            traitFilter = tag.GetTag();
         }
      );
   });

   if (rateFilter == Rate::Auto or not index or not traitFilter or not dataFilter)
      return;

   // Search for the symbol                                             
   const auto found = GetSymbol(traitFilter, dataFilter, rateFilter, index);
   if (found)
      verb << found;
}

/// An arithmetic verb implementation                                         
/// Uses a pattern to modify all output symbols                               
///   @param verb the verb to satisfy                                         
///   @param pos the positive pattern                                         
///   @param neg the negative pattern (optional)                              
///   @param unary the unary pattern (optional)                               
void Node::ArithmeticVerb(Verb& verb, Token const& pos, Token const& neg, Token const& unary) {
   if (verb.GetMass() == 0)
      return;

   const bool inverse = verb.GetMass() < 0;
   bool success {};
   if (not verb and inverse and unary.size()) {
      // No argument, so an unary minus sign                            
      ForEachOutput([&success,&unary](Symbol& symbol) {
         symbol.mCode = Text::Format(unary, symbol.mCode);
         success = true;
      });

      if (not success)
         Logger::Warning(this, ": Unable to ", verb);
      verb << this;
      return;
   }

   // Scan arguments: anything convertible to GLSL can be added to      
   // output symbols' expressions                                       
   verb.ForEachDeep([&](Many const& group) {
      group.Apply([&](auto const& element) {
         try {
            const auto code = element.AsCast<GLSL>();
            if (not code)
               return;

            if (neg.empty() or not inverse) {
               ForEachOutput([&](Symbol& symbol) {
                  symbol.mCode = Text::Format(pos, symbol.mCode, code);
                  success = true;
               });
            }
            else {
               ForEachOutput([&](Symbol& symbol) {
                  symbol.mCode = Text::Format(neg, symbol.mCode, code);
                  success = true;
               });
            }
         }
         catch (...) { }
      });
   });

   if (not success)
      Logger::Warning(this, ": Unable to ", verb);
   verb << this;
}

/// Add/subtract inputs                                                       
///   @param verb the addition/subtraction verb                               
void Node::Add(Verb& verb) {
   ArithmeticVerb(verb, "({} + {})", "({} - {})", "-{}");
}

/// Multiply/divide inputs                                                    
///   @param verb the multiplication verb                                     
void Node::Multiply(Verb& verb) {
   ArithmeticVerb(verb, "({} * {})", "({} / {})", "1.0 / {}");
}

/// Modulate inputs                                                           
///   @param verb the modulation verb                                         
void Node::Modulate(Verb& verb) {
   ArithmeticVerb(verb, "mod({}, {})");
}

/// Exponentiate inputs                                                       
///   @param verb the exponentiation verb                                     
void Node::Exponent(Verb& verb) {
   ArithmeticVerb(verb, "pow({}, {})");
}

/// Randomize inputs                                                          
///   @param verb the randomization verb                                      
void Node::Randomize(Verb& verb) {
   if (verb.GetMass() == 0)
      return;

   // Collect randomization methods and output types                    
   DMeta otype;
   Text method = "simplex";
   verb.ForEachDeep([&](Many const& group) {
      group.ForEach(
         [&](Text const& token) { method = token; },
         [&](DMeta const& t)    { otype = t; }
      );
   });

   // Randomize each output symbol                                      
   bool success = false;
   ForEachOutput([&](Symbol& symbol) {
      DMeta itype = symbol.mTag.GetType();

      // Call the appropriate noise function                            
      if (otype->template CastsTo<Number>(1)) {
         if (itype->template CastsTo<Number>(2))
            AddDefine("SimplexNoise1", TSimplex<1, 2, float>::template Hash<true>());
         else if (itype->template CastsTo<Number>(3))
            AddDefine("SimplexNoise1", TSimplex<1, 3, float>::template Hash<true>());
         else
            TODO();

         symbol.mCode = Text::Format("SimplexNoise1({})", symbol.mCode);
         success = true;
      }
      else TODO();
   });

   if (not success)
      Logger::Warning(this, ": Unable to ", verb);
   verb << this;
}

/// Get stage from node rate                                                  
///   @return the shader stage that will be used                              
auto Node::GetStage() const -> size_t {
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
   VERBOSE_NODE_TAB("Generating code...");
   for (auto child : mChildren)
      child->Generate();
}

/// Add a definition at the node's rate                                       
///   @param name the definition name (used to detect duplications)           
///   @param code the code snippet to add                                     
void Node::AddDefine(Token const& name, const GLSL& code) {
   mMaterial->AddDefine(mRate, name, code);
}

/// Log the material node hierarchy                                           
void Node::Dump() const {
   if (not mChildren) {
      Logger::Verbose(this);
      return;
   }

   const auto scope = Logger::VerboseTab(this);
   for (auto child : mChildren)
      child->Dump();
}

/// Opens node debugging scope                                                
///   @return the openning string                                             
Text Node::DebugBegin() const {
   Code result;
   result += GetToken();
   result += Code::Operator::OpenScope;
   result += mRate;
   return result;
}

/// Closes node debugging scope                                               
///   @return the closing string                                              
Text Node::DebugEnd() const {
   Code result;
   result += Code::Operator::CloseScope;
   return result;
}

/// Convert a symbol from one type to another in GLSL                         
///   @param tag the tag to convert                                           
///   @param symbol the symbol for the original tag                           
///   @param as the type to convert to                                        
///   @param filler number for filling empty stuff                            
///   @return the new symbol                                                  
GLSL ConvertSymbol(Tag const& tag, const GLSL& symbol, DMeta as, Real filler) {
   auto from = tag.GetType();
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
   constexpr Token MatXtoMatY =
       "{0}({1}[0], 0.0, "
           "{1}[1], 0.0, "
           "{1}[2], 0.0, "
           "{1}[3], {2})";
   constexpr Token Mat2toMat4 =
      "mat4({1}[0], 0.0, 0.0, "
           "{1}[1], 0.0, 0.0, "
           "{1}[2], 0.0, 0.0, "
           "{1}[3], 0.0, {2})";

   // Matrix types                                                      
   if (from->template CastsTo<AM4>()) {
      if (as->template CastsTo<AM3>())
         return Text::Format(TypeX, "mat3", symbol);
      else if (as->template CastsTo<AM2>())
         return Text::Format(TypeX, "mat2", symbol);
   }
   else if (from->template CastsTo<AM3>()) {
      if (as->template CastsTo<AM4>())
         return Text::Format(MatXtoMatY, "mat4", symbol, filler);
      else if (as->template CastsTo<AM2>())
         return Text::Format(TypeX, "mat2", symbol);
   }
   else if (from->template CastsTo<AM2>()) {
      if (as->template CastsTo<AM4>())
         return Text::Format(Mat2toMat4, symbol, filler);
      else if (as->template CastsTo<AM3>())
         return Text::Format(MatXtoMatY, "mat3", symbol, filler);
   }
   // Vector types                                                      
   else if (from->template CastsTo<AV4>()) {
      if (as->template CastsTo<AV4>())
         return symbol;
      else if (as->template CastsTo<AV3>())
         return {symbol, ".xyz"};
      else if (as->template CastsTo<AV2>())
         return {symbol, ".xy"};
      else if (as->template CastsTo<AV1>() or as->template CastsTo<A::Number>())
         return {symbol, ".x"};
   }
   else if (from->template CastsTo<AV3>()) {
      if (as->template CastsTo<AV4>())
         return {"vec4(", symbol, ", 1.0)"};
      else if (as->template CastsTo<AV3>())
         return symbol;
      else if (as->template CastsTo<AV2>())
         return {symbol, ".xy"};
      else if (as->template CastsTo<AV1>() or as->template CastsTo<A::Number>())
         return {symbol, ".x"};
   }
   else if (from->template CastsTo<AV2>()) {
      if (as->template CastsTo<AV4>())
         return {"vec4(", symbol, ", ", filler, ", 1.0)"};
      else if (as->template CastsTo<AV3>())
         return {"vec3(", symbol, ", ", filler, ")"};
      else if (as->template CastsTo<AV2>())
         return symbol;
      else if (as->template CastsTo<AV1>() or as->template CastsTo<A::Number>())
         return {symbol, ".x"};
   }
   else if (from->template CastsTo<AV1>() or from->template CastsTo<A::Number>()) {
      if (as->template CastsTo<AV4>())
         return {"vec4(vec3(", symbol, "), 1.0)"};
      else if (as->template CastsTo<AV3>())
         return Text::Format(TypeX, "vec3", symbol);
      else if (as->template CastsTo<AV2>())
         return Text::Format(TypeX, "vec2", symbol);
      else if (as->template CastsTo<AV1>() or as->template CastsTo<A::Number>())
         return symbol;
   }

   LglsError("Can't convert symbol `", tag, "` to `", as, '`');
   return {};
}

/// Get a default type of each of the standard tags                           
///   @param tag the tag definition                                           
///   @return the default tag properties                                      
Node::DefaultTag Node::GetDefaultTag(TMeta tag) {
   static TMapUnsorted<TMeta, DefaultTag> properties;

   if (not properties) {
      properties.Merge(MetaTagOf<Tags::Time>(),          DefaultTag {MetaDataOf<Real>(), Rate::Tick});
      properties.Merge(MetaTagOf<Tags::MousePosition>(), DefaultTag {MetaDataOf<Vec2>(), Rate::Tick});
      properties.Merge(MetaTagOf<Tags::MouseScroll>(),   DefaultTag {MetaDataOf<Vec2>(), Rate::Tick});
      properties.Merge(MetaTagOf<Tags::Size>(),          DefaultTag {MetaDataOf<Vec2>(), Rate::Camera});
      properties.Merge(MetaTagOf<Tags::Projection>(),    DefaultTag {MetaDataOf<Mat4>(), Rate::Camera});
      properties.Merge(MetaTagOf<Tags::FOV>(),           DefaultTag {MetaDataOf<Real>(), Rate::Camera});
      properties.Merge(MetaTagOf<Tags::View>(),          DefaultTag {MetaDataOf<Mat4>(), Rate::Level});
      properties.Merge(MetaTagOf<Tags::Image>(),         DefaultTag {MetaDataOf<Things::Image>(), Rate::Renderable});
      properties.Merge(MetaTagOf<Tags::Transform>(),     DefaultTag {MetaDataOf<Mat4>(), Rate::Instance});
      properties.Merge(MetaTagOf<Tags::Place>(),         DefaultTag {MetaDataOf<Vec3>(), Rate::Vertex});
      properties.Merge(MetaTagOf<Tags::Sampler>(),       DefaultTag {MetaDataOf<Vec2>(), Rate::Vertex});
      properties.Merge(MetaTagOf<Tags::Aim>(),           DefaultTag {MetaDataOf<Vec3>(), Rate::Vertex});
      properties.Merge(MetaTagOf<Tags::Color>(),         DefaultTag {MetaDataOf<Vec4>(), Rate::Vertex});
   }

   auto found = properties.Find(tag);
   if (found)
      return *properties.GetVal(found);

   LglsError("Undefined default tag: ", tag);
   return {};
}

/// Decay a complex type to a fundamental GLSL type                           
///   @param meta the type to decay                                           
///   @return the decayed type                                                
DMeta Node::DecayToGLSLType(DMeta meta) {
   if      (meta.CastsTo(MetaDataOf<double>(), 4)) return MetaDataOf<Vec4d>();
   else if (meta.CastsTo(MetaDataOf<double>(), 3)) return MetaDataOf<Vec3d>();
   else if (meta.CastsTo(MetaDataOf<double>(), 2)) return MetaDataOf<Vec2d>();
   else if (meta.CastsTo(MetaDataOf<double>(), 1)) return MetaDataOf<double>();

   else if (meta.CastsTo(MetaDataOf<Number>(), 4)) return MetaDataOf<Vec4f>();
   else if (meta.CastsTo(MetaDataOf<Number>(), 3)) return MetaDataOf<Vec3f>();
   else if (meta.CastsTo(MetaDataOf<Number>(), 2)) return MetaDataOf<Vec2f>();
   else if (meta.CastsTo(MetaDataOf<Number>(), 1)) return MetaDataOf<float>();

   LglsError("Can't decay type: ", meta);
   return nullptr;
}