///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "FBM.hpp"

using namespace Nodes;


/// FBM function template                                                     
///   @param {0} - unique ID for the FBM function, used only in case of       
///                multiple FBM nodes in hierarchy                            
///   @param {1} - argument(s) for the FBM function                           
///   @param {2} - octave code                                                
constexpr Token FBMTemplate = R"shader(
   float FBM{0}({1}) {{
      const mat2 m = mat2(1.6, 1.2, -1.2, 1.6);
      float f = 0.0;
      {2}
      return f;
   }}
)shader";

/// FBM octave template                                                       
///   @param {0} - mass trait for the current octave                          
///   @param {1} - octave code to execute                                     
constexpr Token FBMOctave = R"shader(
      f += {0} * {1};
)shader";

/// FBM rotate template                                                       
///   @param {0} - place trait to rotate for next octave                      
constexpr Token FBMRotate = R"shader(
      {0} = m * {0};
)shader";


/// FBM node descriptor-constructor                                           
///   @param desc - node descriptor                                           
FBM::FBM(const Descriptor& desc)
   : Node {MetaOf<FBM>(), desc} {
   // Extract number of octaves                                         
   mDescriptor.ExtractTrait<Traits::Count>(mOctaveCount);
   LANGULUS_ASSERT(mOctaveCount >= 1, Material,
      "Bad FBM cycle count", mOctaveCount);

   // Extract base weight                                               
   mDescriptor.ExtractTrait<Traits::Mass>(mBaseWeight);
   LANGULUS_ASSERT(mBaseWeight != 0, Material,
      "Base weight is zero", mOctaveCount);

   // Extract octave code                                               
   // That code also contains all required variables in the form of     
   // selection verbs                                                   
   mDescriptor.ExtractData(mCode);
   LANGULUS_ASSERT(!mCode.IsEmpty(), Material,
      "No FBM octave code");
}

/// For logging                                                               
FBM::operator Debug() const {
   Code result;
   result += Node::DebugBegin();
      result += mOctaveCount;
      result += " octaves of ";
      result += mCode;
   result += Node::DebugEnd();
   return result;
}

/// Generate FBM definition code                                              
void FBM::GenerateDefinition() {
   // FBM takes place as input                                          
   auto symPos = GetSymbol<Traits::Place, Vec2>(mRate);
   LANGULUS_ASSERT(!symPos.IsEmpty(), Material, "No position for FBM");

   // Generate octaves                                                  
   Real f {mBaseWeight};
   GLSL octaves;
   for (Offset i = 0; i < mOctaveCount && mBaseWeight != 0; ++i) {
      // Make a temporary node for each octave, we don't want any       
      // persistent side effects from executing the code here           
      auto temporary = Node::Value::Local(*this);

      // Update inputs for each octave, octave code might use them      
      temporary.mInputs[Trait::From<Traits::Index>()] = i;
      temporary.mInputs[Trait::From<Traits::Mass>()] = f;

      // Run octave code for this node                                  
      temporary.Run(mCode);

      // Generate shader code for octaves                               
      octaves += TemplateFill(FBMOctave, f, temporary.GetOutputSymbol());
      if (i < mOctaveCount - 1)
         octaves += TemplateFill(FBMRotate, symPos);
      f *= mBaseWeight;
   }

   // Commit                                                            
   GLSL definition = TemplateFill(FBMTemplate, "", "", octaves);
   Commit(ShaderToken::Functions, definition);
}

/// Generate FBM definition code                                              
void FBM::GenerateUsage() {
   const GLSL use = "FBM_" + GetNodeID() + "(" + mCodePerUse + ")";
   mOutputs.Insert(Trait::From<Traits::Color, Real>(), use);
}

/// Generate the shader stages                                                
void FBM::Generate() {
   VERBOSE_NODE("Generating code...");
   Descend();
   Consume();
   GenerateDefinition();
   GenerateUsage();
}
