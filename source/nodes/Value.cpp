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
Value Value::Input(Node* parent, const Trait& trait, Rate rate, const GLSL& name) {
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
}

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
Value Value::Output(Node* parent, const Trait& trait, Rate rate, const GLSL& name) {
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
}

///                                                                           
Value Value::Local(Node* parent, const Trait& trait, Rate rate, const GLSL& name) {
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
}

/// Decide trait type and symbol if not decided yet                           
void Value::AutoCompleteTrait() {
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
}

/// For logging and serialization                                             
Value::operator Debug() const {
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
}

/// Project the value by multiplying to a matrix                              
/// If value is smaller than the matrix ranks, it will be filled with 1       
///   @param verb - the projector                                             
void Value::Project(Verb& verb) {
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
}

/// Attempt selecting traits inside this value, or any of its parents         
///   @param verb - the selector                                              
void Value::Select(Verb& verb) {
   Trait found;
   GLSL symbol;
   verb.GetArgument().ForEachDeep([&](const Block& group) {
      EitherDoThis
         group.ForEach([&](const TraitID& trait) {
            symbol = SelectMember(trait, found);
            return symbol.IsEmpty();
         })
      OrThis
         group.ForEach([&](const Trait& trait) {
            symbol = SelectMember(trait.GetTraitID(), found);
            return symbol.IsEmpty();
         });
      return symbol.IsEmpty();
   });

   if (!symbol.IsEmpty()) {
      auto newValue = Ptr<MaterialNodeValue>::New(
         MaterialNodeValue::Local(this, found, mRate, symbol));
      verb << newValue.Get();
      VERBOSE_NODE("Selected: ", Logger::Cyan, symbol);
   }
}

/// Add/subtract inputs                                                       
///   @param verb - the addition/subtraction verb                             
void Value::Add(Verb& verb) {
   if (verb.GetArgument().IsEmpty() && verb.GetMass() < 0) {
      // Invert the input                                               
      mUse = "-(" + GetOutputSymbol() + ")";
      mOutputs.GetValue(0) = mUse;
      VERBOSE_NODE("Inverted (-): ", Logger::Cyan, mUse);
      verb << this;
      return;
   }

   // Gather numbers                                                    
   real numbers = 0;
   auto found = verb.GetArgument().ForEach([&](const real& number) {
      numbers += number;
      return true;
   });

   // Gather symbols                                                    
   TAny<GLSL> symbols;
   found += verb.GetArgument().ForEach([&](const MaterialNode& node) {
      symbols << node.GetOutputSymbol();
      return true;
   });

   if (!found)
      return;

   const auto totalNumbers = pcAbs(verb.GetMass()) * numbers;
   if (totalNumbers) {
      // Add numbers                                                    
      mUse = "(" + GetOutputSymbol()
         + (verb.GetMass() > 0 ? " + " : " - ") + numbers + ")";
      mOutputs.GetValue(0) = mUse;
      VERBOSE_NODE("Added/subtracted: ", Logger::Cyan, mUse);
   }

   if (!symbols.IsEmpty()) {
      // Add symbols                                                    
      mUse = "(" + GetOutputSymbol();
      for (const auto& s : symbols)
         mUse += GLSL(verb.GetMass() > 0 ? " + " : " - ") + s;
      mUse += ")";
      mOutputs.GetValue(0) = mUse;
      VERBOSE_NODE("Added/subtracted: ", Logger::Cyan, mUse);
   }

   verb << this;
}

/// Multiply/divide inputs                                                    
///   @param verb - the multiplication verb                                   
void Value::Multiply(Verb& verb) {
   if (verb.GetArgument().IsEmpty() && verb.GetMass() < 0) {
      // Invert the input                                               
      mUse = "1.0 / (" + GetOutputSymbol() + ")";
      mOutputs.GetValue(0) = mUse;
      VERBOSE_NODE("Inverted (/): " << ccCyan << mUse);
      verb << this;
      return;
   }

   real numbers = verb.GetMass();
   TAny<GLSL> symbols;
   bool continueSearching = true;
   pcptr found = 0;

   // Scan for relevant arguments, like numbers or symbols              
   verb.GetArgument().ForEachDeep([&](const Block& group) {
      group.ForEach([&](const real& number) {
         numbers *= number;
         if (0 == numbers) {
            // If a zero exists in a chain of multiplications we can    
            // immediately return                                       
            verb << real(0);
            continueSearching = false;
            return continueSearching;
         }

         ++found;
         return continueSearching;
      });

      // Gather symbols                                                 
      if (continueSearching) {
         group.ForEach([&](const MaterialNode& node) {
            symbols << node.GetOutputSymbol();
            ++found;
            return continueSearching;
         });
      }

      return continueSearching;
   });

   if (verb.IsDone() || !found)
      return;

   // Modify usage                                                      
   if (pcAbs(numbers) != 1) {
      mUse = "(" + GetOutputSymbol() 
         + (verb.GetMass() > 0 ? " * " : " / " ) + numbers + ")";
      mOutputs.GetValue(0) = mUse;
      VERBOSE_NODE("Multiplied/divided: " << ccCyan << mUse);
   }

   if (!symbols.IsEmpty()) {
      // Multiply symbols                                               
      mUse = "(" + GetOutputSymbol();
      for (const auto& s : symbols)
         mUse += GLSL(verb.GetMass() > 0 ? " * " : " / ") + s;
      mUse += ")";
      mOutputs.GetValue(0) = mUse;
      VERBOSE_NODE("Multiplied/divided: ", Logger::Cyan, mUse);
   }

   verb << this;
}

/// Modulate inputs                                                           
///   @param verb - the modulation verb                                       
void Value::Modulate(Verb& verb) {
   real accumulate = verb.GetMass();
   auto found = verb.GetArgument().ForEach([&](const real& number) {
      accumulate = verb.GetMass() * number;
      return true;
   });

   if (!found)
      return;

   // Modify usage                                                      
   mUse = "mod(" + GetOutputSymbol() + ", " + accumulate + ")";
   mOutputs.GetValue(0) = mUse;
   VERBOSE_NODE("Modulated: ", Logger::Cyan, mUse);
   verb << this;
}

/// Exponentiate inputs                                                       
///   @param verb - the exponentiation verb                                   
void Value::Exponent(Verb& verb) {
   real accumulate = verb.GetMass();
   auto found = verb.GetArgument().ForEach([&](const real& number) {
      accumulate *= number;
      return true;
   });

   if (!found)
      return;

   // Modify usage                                                      
   if (pcAbs(accumulate) != 1) {
      mUse = "pow(" + GetOutputSymbol() + ", " + accumulate + ")";
      mOutputs.GetValue(0) = mUse;
      VERBOSE_NODE("Exponentiated: ", Logger::Cyan, mUse);
   }
   verb << this;
}

/// Randomize inputs                                                          
///   @param verb - the randomization verb                                    
void Value::Randomize(Verb& verb) {
   // Input type is the trait type                                      
   const GLSL itype = GLSL::Type(GetOutputTrait().GetMeta());

   // Check verb argument for the output type                           
   GLSL otype;
   verb.GetArgument().ForEachDeep([&otype](const DataID& type) {
      otype = GLSL::Type(type.GetMeta());
   });

   if (otype.IsEmpty())
      otype = itype;

   // Do the thingamagick, calling the appropriate noise/hash function  
   // to modify the interfaced variable                                 
   GLSL noiseFunction;
   if (itype == "float") {
      noiseFunction = "SimplexNoise1D";
      TODO();
   }
   else if (itype == "vec2") {
      noiseFunction = "SimplexNoise2D";
      mDependencies += THoskins<float>::Hash<2, 2, true>();
      mDependencies +=
         "float SimplexNoise2D(in vec2 p) {\n"
         "   const float K1 = 0.366025404; // (sqrt(3)-1)/2;\n"
         "   const float K2 = 0.211324865; // (3-sqrt(3))/6;\n\n"
         "   vec2 i = floor(p + (p.x + p.y) * K1);\n"
         "   vec2 a = p - i + (i.x + i.y) * K2;\n"
         "   vec2 o = (a.x > a.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);\n"
         "   vec2 b = a - o + K2;\n"
         "   vec2 c = a - 1.0 + 2.0 * K2;\n"
         "   vec3 h = max(0.5 - vec3(dot(a, a), dot(b, b), dot(c, c)), 0.0);\n"
         "   vec3 n = h * h * h * h * vec3(\n"
         "      dot(a, HoskinsHash22(i + 0.0)),\n"
         "      dot(b, HoskinsHash22(i + o)),\n"
         "      dot(c, HoskinsHash22(i + 1.0))\n"
         "   );\n"
         "   return dot(n, vec3(70.0));\n"
         "}\n\n";
   }
   else if (itype == "vec3") {
      noiseFunction = "SimplexNoise3D";
      mDependencies += THoskins<float>::Hash<3, 3, true>();
      mDependencies +=
         "float SimplexNoise3D(in vec3 p) {\n"
         "   // 1. find current tetrahedron T and it's four vertices\n"
         "   //    s, s+i1, s+i2, s+1.0 - absolute skewed (integer) coordinates of T vertices\n"
         "   //    x, x1, x2, x3 - unskewed coordinates of p relative to each of T vertices\n"

         "   // calculate s and x\n"
         "   vec3 s = floor(p + dot(p, vec3(F3)));\n"
         "   vec3 x = p - s + dot(s, vec3(G3));\n"

         "   // calculate i1 and i2\n"
         "   vec3 e = step(vec3(0.0), x - x.yzx);\n"
         "   vec3 i1 = e * (1.0 - e.zxy);\n"
         "   vec3 i2 = 1.0 - e.zxy * (1.0 - e);\n"

         "   // x1, x2, x3\n"
         "   vec3 x1 = x - i1 + G3;\n"
         "   vec3 x2 = x - i2 + 2.0 * G3;\n"
         "   vec3 x3 = x - 1.0 + 3.0 * G3;\n"

         "   // 2. find four surflets and store them in d\n"
         "   vec4 w, d;\n"

         "   // calculate surflet weights\n"
         "   w.x = dot(x, x);\n"
         "   w.y = dot(x1, x1);\n"
         "   w.z = dot(x2, x2);\n"
         "   w.w = dot(x3, x3);\n"

         "   // w fades from 0.6 at the center of the surflet to 0.0 at the margin\n"
         "   w = max(0.6 - w, 0.0);\n"

         "   // calculate surflet components\n"
         "   d.x = dot(HoskinsHash33(s), x);\n"
         "   d.y = dot(HoskinsHash33(s + i1), x1);\n"
         "   d.z = dot(HoskinsHash33(s + i2), x2);\n"
         "   d.w = dot(HoskinsHash33(s + 1.0), x3);\n"

         "   // multiply d by w^4\n"
         "   w *= w;\n"
         "   w *= w;\n"
         "   d *= w;\n"

         "   // 3. return the sum of the four surflets\n"
         "   return dot(d, vec4(52.0));\n"
         "}\n\n";
   }
   else if (itype == "vec4") {
      noiseFunction = "SimplexNoise4D";
      TODO();
   }
   else TODO();

   // Wrap input into the noise function                                
   mUse = noiseFunction + "(" + GetOutputSymbol() + ")";
   if (otype != "float") {
      // Cast the noise function output if required                     
      mUse = otype + "(" + mUse + ")";
   }

   VERBOSE_NODE("Randomized: ", Logger::Cyan, mUse);
   mOutputs.GetValue(0) = mUse;
   verb << this;
}

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
void Value::Generate() {
   Descend();
}
