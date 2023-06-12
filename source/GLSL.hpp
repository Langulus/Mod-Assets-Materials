#pragma once
#include "Common.hpp"
#include <Anyness/Text.hpp>

LANGULUS_EXCEPTION(GLSL);


///                                                                           
///   GLSL code container & tools                                             
///                                                                           
struct GLSL : Text {
   LANGULUS_BASES(Text);
private:
   /// GLSL template for each programmable shader stage                       
   static constexpr Token Templates[ShaderStage::Counter] = {
      // ShaderStage::Vertex                                            
      R"shader(
      //#VERSION

      //#DEFINES

      //#INPUT

      //#OUTPUT

      //#UNIFORM

      //#FUNCTIONS

      void main () {
         //#TEXTURIZE

         //#COLORIZE

         //#TRANSFORM

         //#POSITION

      }
      )shader",

      // ShaderStage::Geometry                                          
      R"shader(
      //#VERSION

      //#DEFINES

      //#INPUT

      //#OUTPUT

      //#UNIFORM

      //#FUNCTIONS

      void main () {
         //#FOREACHSTART

         //#FOREACHEND

      }
      )shader",

      // ShaderStage::TessCtrl                                          
      "",

      // ShaderStage::TessEval                                          
      "",

      // ShaderStage::Pixel                                             
      R"shader(
      //#VERSION

      //#DEFINES

      //#INPUT

      //#OUTPUT

      //#UNIFORM

      //#FUNCTIONS

      void main () {
         //#TRANSFORM

         //#TEXTURIZE

         //#COLORIZE

      }"
      )shader",

      // ShaderStage::Compute                                           
      ""
   };

public:
   using Text::Text;

   GLSL(const Text&);
   GLSL(Text&&);

   explicit GLSL(const CT::Deep auto&);
   explicit GLSL(DMeta);
   explicit GLSL(TMeta);
   explicit GLSL(CMeta);

   template<CT::DenseNumber T, Count C>
   explicit GLSL(const TVector<T, C>&);
   template<CT::DenseNumber T, Count C, Count R>
   explicit GLSL(const TMatrix<T, C, R>&);
   template<CT::DenseNumber T>
   explicit GLSL(const TQuaternion<T>&);

   NOD() static GLSL Template(Offset);
   NOD() bool IsDefined(const Text&);
   NOD() Text Pretty() const;
   NOD() static GLSL Type(DMeta);

   GLSL& Define(const Text&);
   GLSL& SetVersion(const Text&);

   template<class ANYTHING>
   GLSL& operator += (const ANYTHING&);

   template<CT::Data T>
   NOD() static GLSL Type();
};

LANGULUS(INLINED)
GLSL operator "" _glsl(const char* text, std::size_t size) {
   return GLSL {text, size};
}

#include "GLSL.inl"