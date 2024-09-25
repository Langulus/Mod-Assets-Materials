///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
///                                                                           
#include "FBM.hpp"
#include "Value.hpp"
#include "../Material.hpp"

using namespace Nodes;


/// FBM node descriptor-constructor                                           
///   @param desc - node descriptor                                           
FBM::FBM(Describe&& descriptor)
   : Resolvable {this}
   , Node {*descriptor} {
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
   //LANGULUS_ASSERT(mCode, Material, "No FBM octave code"); //TODO
}

/// For logging                                                               
FBM::operator Text() const {
   Code result;
   result += Node::DebugBegin();
   result += Text {", ", mOctaveCount, " octaves of ", mCode};
   result += Node::DebugEnd();
   return result;
}

/// Generate the FBM function                                                 
///   @return the FBM function template                                       
const Symbol& FBM::Generate() {
   // Generate children first                                           
   Descend();

   // Generate octaves                                                  
   Real f {mBaseWeight};
   GLSL octaves;
   for (Offset i = 0; i < mOctaveCount && mBaseWeight != 0; ++i) {
      // Make a temporary node for each octave, we don't want any       
      // persistent side effects from executing the code here           
      Nodes::Value temporary {this};

      // Update inputs for each octave, octave code might use them      
      temporary.template AddLiteral<Traits::Index>(i);
      temporary.template AddLiteral<Traits::Mass>(f);
      temporary.template AddLocal<Traits::Place>(Vec2 {}, "uv");

      // Run octave code for this node                                  
      temporary.Run(mCode);

      // Generate shader code for octaves                               
      auto& symbol = temporary.Generate();
      octaves += Text::TemplateRt(FBMOctave, f, symbol.mCode);
      if (i < mOctaveCount - 1)
         octaves += FBMRotate;
      f *= mBaseWeight;
   }

   // Define the FBM function                                           
   AddDefine("FBM", Text::TemplateRt(FBMTemplate, "", "", octaves));

   // Expose the FBM function template for use by the other nodes       
   return ExposeData<Real>("FBM({})", Traits::Place::OfType<Vec2>());
}
