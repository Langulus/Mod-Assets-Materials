///                                                                           
/// Langulus::Module::Art::Materials                                          
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
struct GLSL : Text {
   using CTTI_ReflectAs = GLSL;
   using CTTI_Named     = Yes<"GLSL">;
   using CTTI_Bases     = Text;
   
   using DMeta          = RTTI::DMeta;
   using TMeta          = RTTI::TMeta;
   using CMeta          = RTTI::CMeta;
   using VMeta          = RTTI::VMeta;

private:
   /// GLSL template for each programmable shader stage                       
   static constexpr Token Templates[ShaderStage::Counter] = {
      // ShaderStage::Vertex                                            
      #include "templates/VertexShaderTemplate.glsl"
      ,

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

   /// Construction from any kind of text that is an Annies container         
   template<CT::Text T> requires CT::Container<T>
   constexpr GLSL(T&& text) {
      this->Absorb(LglsFwd(text));
   }

   //using Text::operator ==;

   /*explicit GLSL(CT::Deep auto const&);
   explicit GLSL(DMeta meta) : GLSL {GLSL::Type(meta)} {}
   explicit GLSL(TMeta meta) : Text {"tag", meta.GetName()} {}
   explicit GLSL(CMeta) {
      TODO(); //expand the constant
   }

   template<CT::Number T, size_t C>
   explicit GLSL(const TVector<T, C>&);

   template<CT::Number T, size_t C, size_t R>
   explicit GLSL(const TMatrix<T, C, R>&);

   template<CT::Number T>
   explicit GLSL(const TQuaternion<T>&);*/

   static GLSL Template(size_t);
   //static bool IsOperator(char);
   template<CT::NotVoid T>
   static GLSL Type();

   bool IsDefined(Token const&) const;
   auto FindKeyword(Text const&) const -> size_t;
   Text Pretty() const;
   static GLSL Type(DMeta);
   static GLSL Scope(CT::Deep auto const&);

   GLSL& Define(Token const&);
   GLSL& SetVersion(Token const&);
};

namespace Langulus::CTTI
{
   /// The presence of this structure makes GLSL a CT::Serializer             
   template<>
   struct Serializer<GLSL> : Serializer<Annies::Text> {
      // GLSL serializer can't be lossy - it's isomorphic               
      static constexpr bool CriticalFailure = true;
      static constexpr bool SkipElements = false;
   };

   /// Rule for serializing GLSL to Text. Wraps it in GLSL{} symbols.         
   template<>
   struct SerializationRule<Annies::Text, GLSL> {
      using S = Serializer<Annies::Text>;
      using Context = typename S::Context;

      static void Serialize(GLSL const& item, Annies::Text& out, Context*) {
         out += "GLSL";
         out += Serial::OpenCode.Token;
         out += item;
         out += Serial::CloseCode.Token;
      }
   };
}

/*namespace Langulus
{
   /// Make a GLSL literal                                                    
   GLSL operator ""_glsl(const char* text, ::std::size_t size) {
      return Annies::Text::FromText(text, size);
   }
}*/

/// Convert bool -> GLSL                                                      
LANGULUS_MORPHISM_CUSTOM(bool, { return from ? "true" : "false"; }, GLSL);

/// Convert Deep -> GLSL                                                      
LANGULUS_MORPHISM_CONCEPT_CUSTOM(CT::Deep, { return GLSL::Scope(from); }, GLSL);

/// Convert DMeta -> GLSL                                                     
LANGULUS_MORPHISM_CUSTOM(RTTI::DMeta, { return GLSL::Type(from); }, GLSL);

/// Convert TMeta -> GLSL                                                     
LANGULUS_MORPHISM_CUSTOM(RTTI::TMeta, { return Annies::Text("tag") + from.GetName(); }, GLSL);

/// Convert CMeta -> GLSL                                                     
LANGULUS_MORPHISM_CUSTOM(RTTI::CMeta, { TODO(); return from.GetName(); }, GLSL);