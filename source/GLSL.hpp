///                                                                           
/// Langulus::Module::Art::Materials                                          
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
///                                                                           
#pragma once
#include "Export.hpp"
#include <Langulus/CT/Quaternion.hpp>
#include <Langulus/CT/Matrix.hpp>
#include <Langulus/CT/CustomVector.hpp>


///                                                                           
///   GLSL code container & tools                                             
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
   using Text::operator ==;

   /// Construction from any kind of text that is an Annies container         
   template<CT::Text T> requires CT::Container<T>
   constexpr GLSL(T&& text) {
      this->Absorb(LglsFwd(text));
   }

   /// Fill template arguments by first serializing them to GLSL           
   ///   @param format the template string                                 
   ///   @param args... the arguments                                      
   ///   @return the resulting text                                        
   static GLSL Format(Token const& format, auto&&...args) {
      return Text::Format(format, Convert<GLSL>(args)...);
   }

   static GLSL Template(size_t);
   //static bool IsOperator(char);

   template<CT::NotVoid T>
   static GLSL FromType();
   static GLSL FromType(DMeta);

   template<CT::DefineTag T>
   static GLSL FromTag();
   static GLSL FromTag(TMeta);

   //static GLSL FromScope(CT::Deep auto const&);
   static GLSL FromQuaternion(CT::Quaternion auto const&);
   static GLSL FromMatrix(CT::Matrix auto const&);
   static GLSL FromVector(CT::CustomVector auto const&);

   bool IsDefined(Token const&) const;
   auto FindKeyword(Text const&) const -> size_t;
   Text Pretty() const;

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

namespace Langulus
{
   /// Make a GLSL literal                                                    
   constexpr GLSL operator ""_glsl(const char* text, ::std::size_t size) {
      return Annies::Text::FromText(text, size);
   }
}

/// Convert std::string_view -> GLSL                                          
LANGULUS_MORPHISM(std::string_view, GLSL);

/// Convert Text -> GLSL                                                      
LANGULUS_MORPHISM(Annies::Text, GLSL);

/// Convert bool -> GLSL                                                      
LANGULUS_MORPHISM_CUSTOM(bool, { return from ? "true" : "false"; }, GLSL);

/// Convert Deep -> GLSL                                                      
//LANGULUS_MORPHISM_CONCEPT_CUSTOM(CT::Deep, { return GLSL::FromScope(from); }, GLSL);

/// Convert DMeta -> GLSL                                                     
LANGULUS_MORPHISM_CUSTOM(RTTI::DMeta, { return GLSL::FromType(from); }, GLSL);

/// Convert TMeta -> GLSL                                                     
LANGULUS_MORPHISM_CUSTOM(RTTI::TMeta, { return GLSL::FromTag(from); }, GLSL);

/// Convert CMeta -> GLSL                                                     
LANGULUS_MORPHISM_CUSTOM(RTTI::CMeta, { TODO(); return from.GetName(); }, GLSL);

/// Convert Number -> GLSL                                                    
LANGULUS_MORPHISM_CONCEPT_CUSTOM(CT::BuiltinNumberUnambiguously, {
      return Annies::Text::FromNumber(from);
   },
   GLSL
);

/// Convert RRate -> GLSL                                                     
LANGULUS_MORPHISM_CUSTOM(RRate, {
      return Annies::Text::NamedLast(from);
   },
   GLSL
);

#include "SerializeGLSL.inl"
