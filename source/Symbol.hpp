///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
///                                                                           
#pragma once
#include "GLSL.hpp"
#include <Langulus/Flow/Rate.hpp>


///                                                                           
///   A shader code symbol                                                    
///                                                                           
/// Transfers data from one node to another. Can be an expression, a literal, 
/// a name to a variable, or a template to a function call                    
///                                                                           
struct Symbol {
   // The rate at which this symbol is recomputed                       
   RefreshRate mRate = Rate::Auto;

   // The trait (if any), the data type (if any), and the value (if     
   // the symbol is a constant/literal). If this symbol represents a    
   // function call, then this is its return type                       
   Trait mTrait;

   // The generated code for the symbol. Will contain a template, if    
   // this symbol is for a function call                                
   GLSL mCode;

   // Number of elements, if symbol is an array                         
   Count mCount = 1;

   // List of arguments, in case this symbol is a function call template
   // One must TemplateFill mCode with these traits to instantiate the  
   // symbol                                                            
   TMany<Trait> mArguments;

   // Number of times a symbol is used                                  
   // If an expression with many uses, the symbol will be moved to a    
   // variable, that will be used instead                               
   Count mUses = 1;

public:
   constexpr Symbol() = default;
   Symbol(const Symbol& other)
      : Symbol {Refer(other)} {}
   Symbol(Symbol&& other) noexcept
      : Symbol {Move(other)} {}

   template<CT::Intent S> requires CT::Exact<TypeOf<S>, Symbol>
   Symbol(S&& other)
      : mRate {other->mRate}
      , mTrait {S::Nest(other->mTrait)}
      , mCode {S::Nest(other->mCode)}
      , mCount {other->mCount}
      , mArguments {S::Nest(other->mArguments)} {}

   template<CT::Data, class...ARGS>
   static Symbol Function(RefreshRate, const Token&, ARGS&&...);

   template<CT::Trait, CT::Data D>
   static Symbol Literal(RefreshRate, D&&);

   template<CT::Trait, CT::Data D>
   static Symbol Variable(RefreshRate, D&&, const Token&);

   bool MatchesFilter(DMeta, RefreshRate) const noexcept;

   GLSL Generate(const Node*) const;

protected:
   void PushArgument(DMeta&&);
   void PushArgument(TMeta&&);
   void PushArgument(Trait&&);
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

} // namespace fmt