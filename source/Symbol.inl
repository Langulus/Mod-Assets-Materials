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
///   @param  rate - the refresh rate of the symbol                           
///   @param  pattern - the template of the function (libfmt format string)   
///   @param ... arguments - the arguments for the pattern                    
///   @return the symbol instance                                             
template<CT::Data T, class... ARGS>
Symbol Symbol::Function(RefreshRate rate, const Token& pattern, ARGS&&... arguments) {
   LANGULUS_ASSUME(DevAssumes, rate != Rate::Auto, "Rate should be resolved");
   (void)Text::TemplateCheck(pattern, arguments...);

   Symbol s;
   s.mRate = rate;
   s.mTrait.SetType<T>();
   s.mCode = pattern;
   (s.PushArgument(Forward<ARGS>(arguments)), ...);
   return s;
}

/// Create a symbol for a literal constant                                    
///   @tparam T - type of the trait associated with the literal               
///   @tparam D - type of data (deducible)                                    
///   @param rate - the refresh rate of the symbol                            
///   @param value - the value of the literal                                 
///   @return the symbol instance                                             
template<CT::Trait T, CT::Data D>
Symbol Symbol::Literal(RefreshRate rate, D&& value) {
   LANGULUS_ASSUME(DevAssumes, rate != Rate::Auto, "Rate should be resolved");

   Symbol s;
   s.mRate = rate;
   s.mTrait = T {Forward<D>(value)};
   return s;
}

/// Create a symbol for a variable                                            
///   @tparam T - type of the trait associated with the variable              
///   @tparam D - type of data (deducible)                                    
///   @param rate - the refresh rate of the symbol                            
///   @param value - the initial value of the variable                        
///   @param name - the variable token                                        
///   @return the symbol instance                                             
template<CT::Trait T, CT::Data D>
Symbol Symbol::Variable(RefreshRate rate, D&& value, const Token& name) {
   LANGULUS_ASSUME(DevAssumes, rate != Rate::Auto, "Rate should be resolved");

   Symbol s;
   s.mRate = rate;
   s.mTrait = T {Forward<D>(value)};
   s.mCode = name;
   return s;
}

/// Check if symbol matches data and/or rate filter                           
///   @param d - data type to be similar to (use nullptr for no filter)       
///   @param r - the rate this symbol needs to be at (or lower)               
///              use Rate::Auto for no filter                                 
///   @return true if this symbol matches the filter requirements             
LANGULUS(INLINED)
bool Symbol::MatchesFilter(DMeta d, RefreshRate r) const noexcept {
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
