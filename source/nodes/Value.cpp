///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Value.hpp"
#include "../Material.hpp"

using namespace Nodes;


/// Value node descriptor-constructor                                         
///   @param desc - the node descriptor                                       
Value::Value(const Descriptor& desc)
   : Node {MetaOf<Value>(), desc} {}

///                                                                           
/*Value Value::Input(Material* producer, const Trait& trait, Rate rate, const GLSL& name) {
   Value result(producer);
   result.mType = ValueType::Input;
   result.mTrait = trait;
   result.mRate = rate;
   result.mName = name;
   result.AutoCompleteTrait();
   if (result.mName.IsEmpty())
      result.mName = producer->AddInput(RRate::PerAuto, result.mTrait, false);
   if (result.mName.IsEmpty())
      throw Except::Content(pcLogFuncError << "Bad name");
   result.mOutputs.Add(result.mTrait, result.mName);
   return result;
}*/

///                                                                           
/*Value Value::Input(Node* parent, const Trait& trait, Rate rate, const GLSL& name) {
   Value result {parent};
   result.mType = ValueType::Input;
   result.mTrait = trait;
   result.mRate = rate;
   result.mName = name;
   result.AutoCompleteTrait();
   if (result.mName.IsEmpty())
      result.mName = result.GetMaterial()->AddInput(Rate::Auto, result.mTrait, false);
   LANGULUS_ASSERT(!result.mName.IsEmpty(), Material, "Bad output symbol");
   result.mOutputs.Insert(result.mTrait, result.mName);
   return result;
}*/

///                                                                           
/*Value Value::Output(Material* producer, const Trait& trait, Rate rate, const GLSL& name) {
   Value result(producer);
   result.mType = ValueType::Output;
   result.mTrait = trait;
   result.mRate = rate;
   result.mName = name;
   result.AutoCompleteTrait();
   if (result.mName.IsEmpty())
      result.mName = producer->AddOutput(result.mRate, result.mTrait, false);
   if (result.mName.IsEmpty())
      throw Except::Content(pcLogFuncError << "Bad name");
   result.mOutputs.Add(result.mTrait, result.mName);
   return result;
}*/

///                                                                           
/*Value Value::Output(Node* parent, const Trait& trait, Rate rate, const GLSL& name) {
   Value result {parent};
   result.mType = ValueType::Output;
   result.mTrait = trait;
   result.mRate = rate;
   result.mName = name;
   result.AutoCompleteTrait();
   if (result.mName.IsEmpty())
      result.mName = result.GetMaterial()->AddOutput(result.mRate, result.mTrait, false);
   LANGULUS_ASSERT(!result.mName.IsEmpty(), Material, "Bad output symbol");
   result.mOutputs.Insert(result.mTrait, result.mName);
   return result;
}*/

///                                                                           
/*Value Value::Local(Node* parent, const Trait& trait, Rate rate, const GLSL& name) {
   Value result {parent};
   result.mType = ValueType::Input;
   result.mTrait = trait;
   result.mRate = rate;
   result.mName = name;
   if (!result.mTrait.IsEmpty())
      result.AutoCompleteTrait();
   if (!result.mName.IsEmpty())
      result.mOutputs.Insert(result.mTrait, result.mName);
   return result;
}

/// Bind to another node's output, or to an input/uniform                     
///   @param trait - the trait to search for                                  
///   @param source - the source node - will be added globally as an          
///                   input or a uniform if nullptr                           
void Value::BindTo(const Trait& trait, const Node* source) {
   mType = ValueType::Input;
   mTrait = trait;

   AutoCompleteTrait();

   if (source) {
      mName = source->GetOutputSymbol(mTrait);
      mRate = source->GetRate();
   }
   else {
      mName = GetMaterial()->AddInput(Rate::Auto, mTrait, false);
      mRate = DefaultTraitRate(trait.GetTrait());
   }

   mOutputs.Reset();
   mOutputs.Insert(mTrait, mName);
}*/

/// Decide trait type and symbol if not decided yet                           
/*void Value::AutoCompleteTrait() {
   if (mTrait.IsUntyped()) {
      // An input must always be declared with a type                   
      // If no such type is provided, try adding a built-in one         
      mTrait.SetType(DefaultTraitType(mTrait.GetTrait()));
   }
   else if (mTrait.Is<DMeta>() && !mTrait.IsEmpty()) {
      // Data ID provided, so make a new trait of this type             
      mTrait = Trait::FromMeta(mTrait.GetTrait(), mTrait.Get<DMeta>());
   }
   else if (!mTrait.IsEmpty()) {
      // Constant provided, just set name to it                         
      mName = Serialize<GLSL>(mTrait);
   }
}*/

/// For logging and serialization                                             
/*Value::operator Debug() const {
   Code result;
   result += Node::DebugBegin();
      result += mTrait.GetTrait();
      result += ", ";
      result += mTrait.GetType();
      if (!mOutputs.IsEmpty()) {
         result += ", ";
         result += GetOutputSymbol();
      }
   result += Node::DebugEnd();
   return result;
}

/// Select a member inside this input by scanning reflected members           
///   @param trait - the trait to select                                      
///   @param found - [out] the trait type is carried inside                   
///   @return the symbol of the found trait (empty if nothing was found)      
GLSL Value::SelectMember(TMeta trait, Trait& found) {
   VERBOSE_NODE("Searching for member ", trait, " inside ", *this);
   for (auto& member : mTrait.GetMeta()->GetMemberList()) {
      if (member.mStaticMember.mTrait == trait) {
         GLSL symbol = trait.GetMeta()->GetToken();
         found = Trait::FromMeta(trait.GetMeta(), member.mType);
         if (mUse.IsEmpty())
            return mName + "." + symbol.Lowercase();
         return "(" + mUse + ")." + symbol.Lowercase();
      }
   }

   return {};
}

/// Get declaration for the input variable                                    
///   @return a variable declaration that is similar to the input             
GLSL Value::GetDeclaration() const {
   return GLSL::Type(mTrait.GetType()) + " " + mName;
}

TMeta Value::GetTrait() const noexcept {
   return mTrait.GetTrait();
}

bool Value::IsValid() const {
   return !mName.IsEmpty();
}*/

/// Project the value by multiplying to a matrix                              
/// If value is smaller than the matrix ranks, it will be filled with 1       
///   @param verb - the projector                                             
/*void Value::Project(Verb& verb) {
   DMeta matrixType = nullptr;
   GLSL transfomation;
   verb.GetArgument().ForEachDeep([&](const TraitID& t) {
      if (!transfomation.IsEmpty())
         transfomation = "(" + transfomation + ") * ";

      auto symbol = GetValue(t.GetMeta(), nullptr, verb.GetFrequency());
      if (!symbol.IsValid())
         throw Except::Content(pcLogSelfError
            << "Bad input/uniform from " << t);
      if (!symbol.GetOutputTrait().GetMeta()->InterpretsAs<AMatrix>())
         throw Except::Content(pcLogSelfError
            << "Can't project via a non-matrix type " << symbol);
      transfomation += symbol.GetOutputSymbol();
      matrixType = symbol.GetOutputTrait().GetMeta();
   });

   if (transfomation.IsEmpty())
      return;

   if (matrixType->InterpretsAs<TSizedMatrix<4>>())
      mUse = "((" + transfomation + ") * " + GetOutputSymbolAs(MetaData::Of<vec4>(), 0) + ")";
   else if (matrixType->InterpretsAs<TSizedMatrix<3>>())
      mUse = "((" + transfomation + ") * " + GetOutputSymbolAs(MetaData::Of<vec3>(), 0) + ")";
   else if (matrixType->InterpretsAs<TSizedMatrix<2>>())
      mUse = "((" + transfomation + ") * " + GetOutputSymbolAs(MetaData::Of<vec2>(), 0) + ")";
   else throw Except::Content(pcLogSelfError
      << "Can't project with unsupported matrix type " << matrixType);

   mOutputs.GetValue(0) = mUse;
   VERBOSE_NODE("Projected: ", Logger::Cyan, mUse);
   verb << this;
}*/


/// Execute a GASM script in the context of the value node                    
///   @param code - the code to execute                                       
/*void Value::DoGASM(const GASM& code) {
   pcLogSelfVerbose << "Executing: " << code << " inside " << this;
   auto parsed = code.Parse();
   Any context {GetBlock()};
   if (!Verb::ExecuteScope(context, parsed)) {
      throw Except::Content(pcLogSelfError
         << "Couldn't execute keyframe code: " << parsed);
   }
}*/

/// Generate the shader stages                                                
Symbol& Value::Generate() {
   Descend();
   TODO();
}
