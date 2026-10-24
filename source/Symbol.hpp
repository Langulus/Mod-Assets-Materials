///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
///                                                                           
#pragma once
#include "GLSL.hpp"
#include <Langulus/Rate.hpp>

using RTTI::DMeta;
using RTTI::TMeta;
using Rate = RefreshRate;


///                                                                           
///   A shader code symbol                                                    
///                                                                           
/// Transfers data from one node to another. Can be an expression, a literal, 
/// a name to a variable, or a template to a function call                    
///                                                                           
struct Symbol {
   // The rate at which this symbol is recomputed                       
   Rate mRate = Rate::Auto;

   // The tag (if any), the data type (if any), and the value (if       
   // the symbol is a constant/literal). If this symbol represents a    
   // function call, then this is its return type                       
   Tag mTag;

   // The generated code for the symbol. Will contain a template, if    
   // this symbol is for a function call                                
   GLSL mCode;

   // Number of elements, if symbol is an array                         
   size_t mCount = 1;

   // List of arguments, in case this symbol is a function call template
   // One must TemplateFill mCode with these tags to instantiate the    
   // symbol                                                            
   TMany<Tag> mArguments;

   // Number of times a symbol is used                                  
   // If an expression with many uses, the symbol will be moved to a    
   // variable, that will be used instead                               
   size_t mUses = 1;

public:
   constexpr Symbol() = default;
   Symbol(Symbol const& other)     : Symbol {Refer(other)} {}
   Symbol(Symbol&& other) noexcept : Symbol {Move(other)} {}

   template<template<class> class S> requires CT::Intent<S<Symbol>>
   Symbol(S<Symbol>&& other)
      : mRate     {other->mRate}
      , mTag      {NestIntentOf(other, other->mTag)}
      , mCode     {NestIntentOf(other, other->mCode)}
      , mCount    {other->mCount}
      , mArguments{NestIntentOf(other, other->mArguments)} {}

   template<CT::NotVoid, class...ARGS>
   static Symbol Function(Rate, Token const&, ARGS&&...);

   template<CT::DefineTag, CT::NotVoid D>
   static Symbol Literal(Rate, D&&);

   template<CT::DefineTag, CT::NotVoid D>
   static Symbol Variable(Rate, D&&, Token const&);

   bool MatchesFilter(DMeta, Rate) const noexcept;

   GLSL Generate(const Node*) const;

   /// Should complain at compile-time if format string has mismatching       
   /// number of arguments                                                    
   template<::std::size_t...N> LANGULUS(INLINED)
   static constexpr auto CheckPattern(const Token& f, ::std::index_sequence<N...>) {
      return fmt::format_string<decltype(N)...> {fmt::runtime(f)};
   }

   /// Attempt filling the template, statically checking if argument count is 
   /// satisfied, and other erroneous conditions. Since arguments contain     
   /// unformattable data, that has more to do with seeking actual data from  
   /// the node hierarchy later, arguments are substituted with an index      
   /// sequence.                                                              
   ///   @param format the template string                                    
   ///   @param args... the arguments                                         
   template<class...ARGS> LANGULUS(INLINED)
   static constexpr auto TemplateCheck(const Token& f, ARGS&&...) {
      return CheckPattern(f, ::std::make_index_sequence<sizeof...(ARGS)> {});
   }

protected:
   void PushArgument(DMeta&&);
   void PushArgument(TMeta&&);
   void PushArgument(Tag&&);
};

using Symbols = TMany<Symbol>;

#include "Symbol.inl"

namespace fmt
{
   ///                                                                        
   /// Extend FMT to be capable of log/fill templates with symbols            
   ///                                                                        
   template<>
   struct formatter<Symbol> {
      template<class CONTEXT>
      constexpr auto parse(CONTEXT& ctx) {
         return ctx.begin();
      }

      template<class CONTEXT> LANGULUS(INLINED)
      auto format(Symbol const& element, CONTEXT& ctx) const {
         using namespace Langulus;
         auto asText = element.mCode.operator Token();
         return fmt::format_to(ctx.out(), "{}", asText);
      }
   };
}