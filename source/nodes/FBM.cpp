///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "../Node.hpp"


/// Create an FBM node from a parent node                                     
///   @param parent - the parent node                                         
MaterialNodeFBM::MaterialNodeFBM(MaterialNode* parent)
   : MaterialNode{ MetaData::Of<MaterialNodeFBM>(), parent, {} }
   , mCodePerUse{ parent->GetOutputSymbol() }
   , mArgument{ parent->GetOutputTrait() } {
   mRate = parent->GetRate();
}

/// For logging                                                               
MaterialNodeFBM::operator Debug() const {
   GASM result;
   result += MaterialNode::DebugBegin();
      result += mOctaveCount;
      result += " octaves of ";
      result += mCodePerOctave;
   result += MaterialNode::DebugEnd();
   return result;
}

/// Fractal brownian motion                                                   
///   @param verb - the FBM verb                                                
void MaterialNodeFBM::FBM(Verb& verb) {
   // Determine octave count, based on verb mass                        
   mOctaveCount = static_cast<pcptr>(verb.GetMass());
   if (mOctaveCount < 1) {
      throw Except::Content(pcLogSelfError
         << "Bad FBM cycle count: " << mOctaveCount);
   }

   // Configure the brownian motion based on verb arguments               
   PC_VERBOSE_MATERIAL("FBM: " << verb.GetArgument());
   verb.GetArgument().ForEachDeep([&](const Block& group) {
      EitherDoThis
         group.ForEach([&](const GASM& code) {
            mCodePerOctave = code;
         })
      OrThis
         group.ForEach([&](const MaterialNode& node) {
            mCodePerUse = node.GetOutputSymbol();
         })
      OrThis
         group.ForEach([&](const Construct& construct) {
            mCodePerUse = CodeFromConstruct(construct);
         });
   });
   
   PC_VERBOSE_MATERIAL(ccYellow << "FBM(" << mCodePerUse << ")");
   PC_VERBOSE_MATERIAL(ccYellow << "{ " << mCodePerOctave << " }");
   verb.Done();
}

/// Generate FBM definition code                                                
void MaterialNodeFBM::GenerateDefinition() {
   // Parse the code for a single octave                                 
   auto argument = Ptr<MaterialNodeValue>::New(
      MaterialNodeValue::Local(this, mArgument, mRate, "uv"));
   argument->DoGASM(mCodePerOctave);
   argument->Generate();

   // Generate the FBM function                                          
   real f = 0.5;
   GLSL definition = 
      "float FBM_"_glsl + GetNodeID() + "(" + argument->GetDeclaration() + ") {\n"
      "   float f = " + f + ";\n"
      "   mat2 m = mat2(1.6, 1.2, -1.2, 1.6);\n";

   f *= 0.5;
   for (pcptr i = 0; i < mOctaveCount; ++i) {
      definition += "   f += "_glsl + f + " * " + argument->GetOutputSymbol() + ";\n";
      if (i < mOctaveCount - 1)
         definition += " uv = m * uv;\n";
      f *= 0.5;
   }
   definition += "   return f;\n}\n\n";

   // Commit                                                            
   Commit(ShaderToken::Functions, definition);
}

/// Generate FBM definition code                                                
void MaterialNodeFBM::GenerateUsage() {
   const GLSL use = "FBM_" + GetNodeID() + "(" + mCodePerUse + ")";
   mOutputs.Add(Trait::From<Traits::Color, real>(), use);
}

/// Generate the shader stages                                                
void MaterialNodeFBM::Generate() {
   PC_VERBOSE_MATERIAL("Generating code...");
   Descend();
   Consume();

   GenerateDefinition();
   GenerateUsage();
}
