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
   constexpr auto check = TemplateCheck(pattern, arguments...);

   Symbol s;
   s.mRate = rate;
   s.mTrait.SetType<T>();
   s.mCode = pattern;
   (s.PushArgument(Forward<ARGS>(arguments)), ...);
   return s;
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
