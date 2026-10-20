///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
///                                                                           
#pragma once
#include "Export.hpp"

/*struct GLSL;

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
   }

   /// A GLSL-convertible type is one that has either an implicit or explicit 
   /// cast operator to GLSL type, or can be used to explicitly initialize a  
   /// GLSL container                                                         
   template<class...T>
   concept ConvertibleToGLSL = ((
           Inner::ConvertibleToGLSLByOperator<T>
        or Inner::ConvertibleToGLSLByConstructor<T>) and ...);
}*/


///                                                                           
///   GLSL code container & tools                                             
///                                                                           
struct GLSL : Annies::Text {
   using CTTI_ReflectAs = GLSL;
   using CTTI_Named     = Yes<"GLSL">;
   using CTTI_Bases     = Annies::Text;
   
   using DMeta          = RTTI::DMeta;
   using TMeta          = RTTI::TMeta;
   using CMeta          = RTTI::CMeta;
   using VMeta          = RTTI::VMeta;

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

   //GLSL(Text const&);
   //GLSL(Text&&);

   explicit GLSL(const CT::Deep auto&);
   explicit GLSL(DMeta);
   explicit GLSL(TMeta);
   explicit GLSL(CMeta);

   template<CT::Number T, size_t C>
   explicit GLSL(const TVector<T, C>&);
   template<CT::Number T, size_t C, size_t R>
   explicit GLSL(const TMatrix<T, C, R>&);
   template<CT::Number T>
   explicit GLSL(const TQuaternion<T>&);

   static GLSL Template(size_t);
   static bool IsOperator(char);
   template<CT::NotVoid T>
   static GLSL Type();

   bool IsDefined(Token const&) const;
   Index FindKeyword(Text const&) const;
   Text Pretty() const;
   static GLSL Type(DMeta);

   GLSL& Define(Token const&);
   GLSL& SetVersion(Token const&);

   ///                                                                        
   ///   Concatenation                                                        
   ///                                                                        
   /*template<class T> requires CT::ConvertibleToGLSL<Deint<T>>
   GLSL operator + (T&&) const;

   template<class T> requires CT::ConvertibleToGLSL<Deint<T>>
   GLSL& operator += (T&&);*/
};

namespace Langulus
{
   /// Make a GLSL literal                                                    
   GLSL operator ""_glsl(const char* text, ::std::size_t size) {
      return Annies::Text::From(text, size);
   }
}

#include "GLSL.inl"