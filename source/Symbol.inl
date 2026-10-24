///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
///                                                                           
#pragma once
#include "Symbol.hpp"


/// Create a symbol for a function                                            
///   @tparam T return type of the function                                   
///   @param rate the refresh rate of the symbol                              
///   @param pattern the template of the function (libfmt format string)      
///   @param arguments... the arguments for the pattern                       
///   @return the symbol instance                                             
template<CT::NotVoid T, class...ARGS>
Symbol Symbol::Function(Rate rate, Token const& pattern, ARGS&&...arguments) {
   LglsAssumeDev(rate != Rate::Auto, "Rate should be resolved");
   TemplateCheck(pattern, arguments...);

   Symbol s;
   s.mRate = rate;
   s.mTag.SetType<T>();
   s.mCode = pattern;
   (s.PushArgument(LglsFwd(arguments)), ...);
   return s;
}

/// Create a symbol for a literal constant                                    
///   @tparam T type of the tag associated with the literal                   
///   @param rate the refresh rate of the symbol                              
///   @param value the value of the literal                                   
///   @return the symbol instance                                             
template<CT::DefineTag T, CT::NotVoid D>
Symbol Symbol::Literal(Rate rate, D&& value) {
   LglsAssumeDev(rate != Rate::Auto, "Rate should be resolved");

   Symbol s;
   s.mRate = rate;
   s.mTag = T {LglsFwd(value)};
   return s;
}

/// Create a symbol for a variable                                            
///   @tparam T type of the tag associated with the variable                  
///   @param rate the refresh rate of the symbol                              
///   @param value the initial value of the variable                          
///   @param name the variable token                                          
///   @return the symbol instance                                             
template<CT::DefineTag T, CT::NotVoid D>
Symbol Symbol::Variable(Rate rate, D&& value, Token const& name) {
   LglsAssumeDev(rate != Rate::Auto, "Rate should be resolved");

   Symbol s;
   s.mRate = rate;
   s.mTag = T {LglsFwd(value)};
   s.mCode = name;
   return s;
}

/// Check if symbol matches data and/or rate filter                           
///   @param d data type to be similar to (use {} for no filter)              
///   @param r the rate this symbol needs to be at (or lower)                 
///      use Rate::Auto for no filter                                         
///   @return true if this symbol matches the filter requirements             
inline bool Symbol::MatchesFilter(DMeta d, Rate r) const noexcept {
   LglsAssumeDev(mRate != Rate::Auto, "Rate should be resolved");
   return (!d || mTag.CastsTo(d)) && (r == Rate::Auto || r <= mRate);
}

inline void Symbol::PushArgument(DMeta&& type) {
   mArguments << Tag::Typed({}, type);
}

inline void Symbol::PushArgument(TMeta&& type) {
   mArguments << Tag::Typed(type, {});
}

inline void Symbol::PushArgument(Tag&& type) {
   mArguments << LglsFwd(type);
}
