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
template<class F>
LANGULUS(INLINED)
Count Node::ForEachChild(F&& call) {
   auto counter = mChildren.ForEach(Forward<F>(call));
   for (auto child : mChildren)
      counter += child->ForEachChild(Forward<F>(call));
   return counter;
}

/// Add an output symbol to the node                                          
///   @tparam ...ARGS - optional arguments, if symbol is a function template  
///                     these arguments can be DMetas, or TMetas, or both by  
///                     providing a Trait with a given type                   
///   @tparam T - the data type of the output (or return type of function)    
///   @param pattern - the symbol name/function template                      
///   @param ... - parameters, in case pattern is a function template         
///   @return the symbol handle                                               
template<CT::Data T, class... ARGS>
Symbol& Node::ExposeData(const Token& pattern, ARGS&&...) {
   TODO();
}

/// Add an output symbol to the node                                          
///   @tparam ...ARGS - optional arguments, if symbol is a function template  
///                     these arguments can be DMetas, or TMetas, or both by  
///                     providing a Trait with a given type                   
///   @tparam D - the trait type of the output                                
///   @tparam T - the data type of the output (or return type of function)    
///   @param pattern - the symbol name/function template                      
///   @param ... - parameters, in case pattern is a function template         
///   @return the symbol handle                                               
template<CT::Trait T, CT::Data D, class... ARGS>
Symbol& Node::ExposeTrait(const Token& pattern, ARGS&&...) {
   TODO();
}

/// Select a symbol from the node hierarchy (const)                           
/// Checks local symbols first, then climbs up the hierarchy up to Root, where
/// it starts selecting global material inputs/constants                      
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
/// Checks local symbols first, then climbs up the hierarchy up to Root, where
/// it starts selecting global material inputs/constants                      
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
/// Checks local symbols first, then climbs up the hierarchy up to Root, where
/// it starts selecting global material inputs/constants                      
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
