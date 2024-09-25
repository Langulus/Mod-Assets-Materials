///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
///                                                                           
#pragma once
#include "Common.hpp"

LANGULUS_EXCEPTION(GLSL);

struct GLSL;


namespace Langulus::CT
{
   namespace Inner
   {
   
      /// Do types have an explicit or implicit cast operator to GLSL         
      template<class...T>
      concept ConvertibleToGLSLByOperator = requires (T&...a) {
         ((a.operator GLSL()), ...); };

      /// Does GLSL has an explicit/implicit constructor that accepts T       
      template<class...T>
      concept ConvertibleToGLSLByConstructor = requires (T&...a) {
         ((GLSL {a}), ...); };

   } // namespace Langulus::CT::Inner

   /// A GLSL-convertible type is one that has either an implicit or explicit 
   /// cast operator to GLSL type, or can be used to explicitly initialize a  
   /// GLSL container                                                         
   template<class...T>
   concept ConvertibleToGLSL = ((
           Inner::ConvertibleToGLSLByOperator<T>
        or Inner::ConvertibleToGLSLByConstructor<T>) and ...);

} // namespace Langulus::CT


///                                                                           
///   GLSL code container & tools                                             
///                                                                           
struct GLSL : Text {
   LANGULUS(NAME) "GLSL";
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
   using Text::operator ==;

   GLSL(const Text&);
   GLSL(Text&&);

   explicit GLSL(const CT::Deep auto&);
   explicit GLSL(DMeta);
   explicit GLSL(TMeta);
   explicit GLSL(CMeta);

   template<CT::Number T, Count C>
   explicit GLSL(const TVector<T, C>&);
   template<CT::Number T, Count C, Count R>
   explicit GLSL(const TMatrix<T, C, R>&);
   template<CT::Number T>
   explicit GLSL(const TQuaternion<T>&);

   NOD() static GLSL Template(Offset);
   NOD() static bool IsOperator(char);
   template<CT::Data T>
   NOD() static GLSL Type();

   NOD() bool IsDefined(const Token&) const;
   NOD() Index FindKeyword(const Text&) const;
   NOD() Text Pretty() const;
   NOD() static GLSL Type(DMeta);

   GLSL& Define(const Token&);
   GLSL& SetVersion(const Token&);

   ///                                                                        
   ///   Concatenation                                                        
   ///                                                                        
   template<class T> requires CT::ConvertibleToGLSL<Deint<T>>
   NOD() GLSL operator + (T&&) const;

   template<class T> requires CT::ConvertibleToGLSL<Deint<T>>
   GLSL& operator += (T&&);
};

namespace Langulus
{

   /// Make a GLSL literal                                                    
   LANGULUS(INLINED)
   GLSL operator ""_glsl(const char* text, ::std::size_t size) {
      return Anyness::Text::From(text, size);
   }

} // namespace Langulus

#include "GLSL.inl"