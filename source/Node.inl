///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "Node.hpp"


/// Get the material this node belongs to                                     
///   @return pointer to the material                                         
LANGULUS(INLINED)
Material* Node::GetMaterial() const noexcept {
   return mMaterial;
}

/// Execute a function in each child node                                     
///   @tparam F - function signature (deducible)                              
///   @param call - the function callback                                     
///   @return the number of successulf executions of the call                 
template<class F>
LANGULUS(INLINED)
Count Node::ForEachChild(F&& call) {
   auto counter = mChildren.ForEach(Forward<F>(call));
   for (auto child : mChildren)
      counter += child->ForEachChild(Forward<F>(call));
   return counter;
}

/// Add a local variable to the node, with a specific trait and initial value 
///   @tparam T - type of trait to associate with the value                   
///   @tparam D - data type of the value (deducible)                          
///   @param value - the value itself                                         
///   @param variable - optional name for the variable; if not name is given  
///                     the value will be used as a literal constant          
///   @return reference to the symbol that corresponds to the input           
template<CT::Trait T, CT::Data D>
const Symbol& Node::AddLocal(D&& value, const Token& variable) {
   const auto meta = MetaOf<T>();
   mLocalsT[meta] << Symbol::Variable<T>(mRate, Forward<D>(value), variable);
   return mLocalsT[meta].Last();
}

/// Add a literal to the node, with a specific trait and value                
///   @tparam T - type of trait to associate with the value                   
///   @tparam D - data type of the value (deducible)                          
///   @param value - the value itself                                         
///   @return reference to the symbol that corresponds to the input           
template<CT::Trait T, CT::Data D>
const Symbol& Node::AddLiteral(D&& value) {
   const auto meta = MetaOf<T>();
   mLocalsT[meta] << Symbol::Literal<T>(mRate, Forward<D>(value));
   return mLocalsT[meta].Last();
}

/// Add an output symbol to the node                                          
///   @tparam T - the data type of the output (or return type of function)    
///   @tparam ...ARGS - optional arguments, if symbol is a function template  
///                     these arguments can be DMetas, or TMetas, or both by  
///                     providing a Trait with a given type                   
///   @param pattern - the symbol name/function template                      
///   @param a... - parameters, in case pattern is a function template        
///   @return the symbol handle                                               
template<CT::Data T, class... ARGS>
Symbol& Node::ExposeData(const Token& pattern, ARGS&&... a) {
   const auto meta = MetaOf<T>();
   mLocalsD[meta] << Symbol::Function<T>(mRate, pattern, Forward<ARGS>(a)...);
   return mLocalsD[meta].Last();
}

/// Add an output symbol to the node                                          
///   @tparam ...ARGS - optional arguments, if symbol is a function template  
///                     these arguments can be DMetas, or TMetas, or both by  
///                     providing a Trait with a given type                   
///   @tparam T - the trait type of the output                                
///   @tparam D - the data type of the output (or return type of function)    
///   @param pattern - the symbol name/function template                      
///   @param a... - parameters, in case pattern is a function template        
///   @return the symbol handle                                               
template<CT::Trait T, CT::Data D, class... ARGS>
Symbol& Node::ExposeTrait(const Token& pattern, ARGS&&... a) {
   const auto meta = MetaOf<T>();
   mLocalsT[meta] << Symbol::Function<D>(mRate, pattern, Forward<ARGS>(a)...);
   return mLocalsT[meta].Last();
}

/// Select a symbol from the node hierarchy (const)                           
/// Checks local symbols first, then climbs up the hierarchy up to Root,      
/// where it starts selecting global material inputs/constants                
///   @param t - trait type filter                                            
///   @param d - data type filter                                             
///   @param r - rate filter                                                  
///   @param i - index filter                                                 
///   @return a pointer to the symbol, or nullptr if not found                
LANGULUS(INLINED)
const Symbol* Node::GetSymbol(TMeta t, DMeta d, Rate r, Index i) const {
   return const_cast<Node*>(this)->GetSymbol(t, d, r, i);
}

/// Select a symbol from the node hierarchy                                   
/// Checks local symbols first, then climbs up the hierarchy up to Root,      
/// where it starts selecting global material inputs/constants                
///   @tparam T - trait type filter (use void to disable)                     
///   @tparam D - data type filter (use void to disable)                      
///   @param r - rate filter                                                  
///   @param i - index filter                                                 
///   @return a pointer to the symbol, or nullptr if not found                
template<class T, class D>
LANGULUS(INLINED)
Symbol* Node::GetSymbol(Rate r, Index i) {
   static_assert(CT::Void<T> || CT::Trait<T>,
      "T must be either trait, or void");
   static_assert(CT::Void<D> || CT::Data<D>,
      "D must be either data type, or void");

   return GetSymbol(MetaOf<T>(), MetaOf<D>(), r, i);
}

/// Select a symbol from the node hierarchy (const)                           
/// Checks local symbols first, then climbs up the hierarchy up to Root,      
/// where it starts selecting global material inputs/constants                
///   @tparam T - trait type filter (use void to disable)                     
///   @tparam D - data type filter (use void to disable)                      
///   @param r - rate filter                                                  
///   @param i - index filter                                                 
///   @return a pointer to the symbol, or nullptr if not found                
template<class T, class D>
LANGULUS(INLINED)
const Symbol* Node::GetSymbol(Rate r, Index i) const {
   return const_cast<Node*>(this)->template GetSymbol<T, D>(r, i);
}

/// Get the refresh rate of the node                                          
///   @return the rate of this node                                           
LANGULUS(INLINED)
Rate Node::GetRate() const noexcept {
   return mRate;
}

/// Execute a function for each symbol in the inputs                          
///   @tparam F - function signature (deducible)                              
///   @param call - the function to call for each symbol                      
///   @return the number of execution of call                                 
template<class F>
Count Node::ForEachInput(F&& call) {
   using A = ArgumentOf<F>;
   using R = ReturnOf<F>;
   static_assert(CT::Same<A, Symbol>, "Function argument must be a Symbol");

   Count counter {};
   for (auto pair : mLocalsT) {
      for (auto& symbol : pair.mValue) {
         if constexpr (CT::Bool<R>) {
            if (!call(symbol))
               return counter;
         }
         else call(symbol);
         ++counter;
      }
   }

   for (auto pair : mLocalsD) {
      for (auto& symbol : pair.mValue) {
         if constexpr (CT::Bool<R>) {
            if (!call(symbol))
               return counter;
         }
         else call(symbol);
         ++counter;
      }
   }

   return counter;
}

/// Execute a function for each symbol in the outputs                         
///   @tparam F - function signature (deducible)                              
///   @param call - the function to call for each symbol                      
///   @return the number of execution of call                                 
template<class F>
Count Node::ForEachOutput(F&& call) {
   using A = ArgumentOf<F>;
   using R = ReturnOf<F>;
   static_assert(CT::Same<A, Symbol>, "Function argument must be a Symbol");

   Count counter {};
   for (auto pair : mOutputsT) {
      for (auto& symbol : pair.mValue) {
         if constexpr (CT::Bool<R>) {
            if (!call(symbol))
               return counter;
         }
         else call(symbol);
         ++counter;
      }
   }

   for (auto pair : mOutputsD) {
      for (auto& symbol : pair.mValue) {
         if constexpr (CT::Bool<R>) {
            if (!call(symbol))
               return counter;
         }
         else call(symbol);
         ++counter;
      }
   }

   return counter;
}
