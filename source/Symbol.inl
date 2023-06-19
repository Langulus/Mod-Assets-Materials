///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "Symbol.hpp"

/// Create a symbol for a function                                            
///   @tparam T - return type of the function                                 
///   @tparam ...ARGS - optional arguments for the function                   
///   @param  pattern - the template of the function (libfmt format string)   
///   @param ... arguments - the arguments for the pattern                    
///   @return the symbol instance                                             
template<CT::Data T, class... ARGS>
Symbol Symbol::Function(Rate rate, const Token& pattern, ARGS&&... arguments) {
   LANGULUS_ASSUME(DevAssumes, rate != Rate::Auto, "Rate should be resolved");
   constexpr auto check = TemplateCheck(pattern, arguments...);

   Symbol s;
   s.mRate = rate;
   s.mTrait.SetType<T>();
   s.mCode = pattern;
   (s.PushArgument(Forward<ARGS>(arguments)), ...);
   return s;
}

/// Check if symbol matches data and/or rate filter                           
///   @param d - data type to be similar to (use nullptr for no filter)       
///   @param r - the rate this symbol needs to be at (or lower)               
///              use Rate::Auto for no filter                                 
///   @return true if this symbol matches the filter requirements             
LANGULUS(INLINED)
bool Symbol::MatchesFilter(DMeta d, Rate r) const noexcept {
   LANGULUS_ASSUME(DevAssumes, mRate != Rate::Auto, "Rate should be resolved");
   return (!d || mTrait.CastsToMeta(d)) && (r == Rate::Auto || r <= mRate);
}

LANGULUS(INLINED)
void Symbol::PushArgument(DMeta&& type) {
   mArguments << Trait::FromMeta(nullptr, type);
}

LANGULUS(INLINED)
void Symbol::PushArgument(TMeta&& type) {
   mArguments << Trait::FromMeta(type, nullptr);
}

LANGULUS(INLINED)
void Symbol::PushArgument(Trait&& type) {
   mArguments << Forward<Trait>(type);
}
