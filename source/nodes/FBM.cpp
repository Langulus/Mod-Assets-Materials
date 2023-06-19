///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "FBM.hpp"
#include "Value.hpp"
#include "../Material.hpp"

using namespace Nodes;


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

/// Generate the FBM function                                                 
///   @return the FBM function template                                       
Symbol& FBM::Generate() {
   // Generate children first                                           
   Descend();

   // Generate octaves                                                  
   Real f {mBaseWeight};
   GLSL octaves;
   for (Offset i = 0; i < mOctaveCount && mBaseWeight != 0; ++i) {
      // Make a temporary node for each octave, we don't want any       
      // persistent side effects from executing the code here           
      auto temporary = Nodes::Value::Local(this);

      // Update inputs for each octave, octave code might use them      
      temporary.mInputs[Traits::Index {}] = i;
      temporary.mInputs[Traits::Mass {}] = f;
      temporary.mInputs[Traits::Place {}] = "uv";

      // Run octave code for this node                                  
      temporary.Run(mCode);

      // Generate shader code for octaves                               
      octaves += TemplateFill(FBMOctave, f, temporary.GetOutputSymbol());
      if (i < mOctaveCount - 1)
         octaves += FBMRotate;
      f *= mBaseWeight;
   }

   // Define the FBM function                                           
   AddDefine("FBM", TemplateFill(FBMTemplate, "", "", octaves));

   // Expose the FBM function template for use by the other nodes       
   return Expose<Real>("FBM({})", Traits::Place::OfType<Vec2>());
}
