///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
///                                                                           
#pragma once
#include "Node.hpp"


/// Get the material this node belongs to                                     
///   @return pointer to the material                                         
LANGULUS(INLINED)
Material* Node::GetMaterial() const noexcept {
   return mMaterial;
}

/// Add a child                                                               
///   @attention assumes node is a valid pointer                              
///   @tparam TWOSIDED - true to also set the node's parent;                  
///                      used mainly internally to avoid endless loops        
///   @param node - node instance to add as child                             
///   @return the number of added children                                    
template<bool TWOSIDED>
Count Node::AddChild(Node* node) {
   LANGULUS_ASSUME(UserAssumes, node, "Bad node pointer");
   VERBOSE_NODE("Adding child ", node, " (", node->GetReferences(), " uses)");
   const auto added = mChildren.Merge(IndexBack, node);
   if constexpr (TWOSIDED) {
      if (added and node->mParent != this) {
         if (node->mParent)
            node->mParent->RemoveChild<false>(node);

         node->mParent = this;
         node->mMaterial = GetMaterial();
         VERBOSE_NODE("Now parent of ", node, " (", node->GetReferences(), " uses)");
      }
   }

   return added;
}

/// Remove a child that matches pointer                                       
///   @attention assumes node is a valid pointer                              
///   @tparam TWOSIDED - true to also remove node's owner;                    
///                      used mainly internally to avoid endless loops        
///   @param node - node instance to remove from children                     
///   @return the number of removed children                                  
template<bool TWOSIDED>
Count Node::RemoveChild(Node* node) {
   LANGULUS_ASSUME(UserAssumes, node, "Bad node pointer");
   VERBOSE_NODE("Removing child ", node, " (", node->GetReferences(), " uses)");
   const auto removed = mChildren.Remove(node);
   if constexpr (TWOSIDED) {
      if (removed and node->mParent == this) {
         node->mParent = nullptr;
         node->mMaterial = nullptr;
      }
   }

   return removed;
}

/// Execute a function in each child node                                     
///   @tparam F - function signature (deducible)                              
///   @param call - the function callback                                     
///   @return the number of successulf executions of the call                 
template<class F>
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
auto Node::AddLocal(D&& value, const Token& variable) -> const Symbol& {
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
auto Node::AddLiteral(D&& value) -> const Symbol& {
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
auto Node::ExposeData(const Token& pattern, ARGS&&... a) -> Symbol& {
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
auto Node::ExposeTrait(const Token& pattern, ARGS&&... a) -> Symbol& {
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
auto Node::GetSymbol(TMeta t, DMeta d, RefreshRate r, Index i) const -> const Symbol* {
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
template<class T, class D> LANGULUS(INLINED)
auto Node::GetSymbol(RefreshRate r, Index i) -> Symbol* {
   static_assert(CT::Void<T> or CT::Trait<T>,
      "T must be either trait, or void");
   static_assert(CT::Void<D> or CT::Data<D>,
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
template<class T, class D> LANGULUS(INLINED)
auto Node::GetSymbol(RefreshRate r, Index i) const -> const Symbol* {
   return const_cast<Node*>(this)->template GetSymbol<T, D>(r, i);
}

/// Get the refresh rate of the node                                          
///   @return the rate of this node                                           
LANGULUS(INLINED)
auto Node::GetRate() const noexcept -> RefreshRate {
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
            if (not call(symbol))
               return counter;
         }
         else call(symbol);
         ++counter;
      }
   }

   for (auto pair : mLocalsD) {
      for (auto& symbol : pair.mValue) {
         if constexpr (CT::Bool<R>) {
            if (not call(symbol))
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
            if (not call(symbol))
               return counter;
         }
         else call(symbol);
         ++counter;
      }
   }

   for (auto pair : mOutputsD) {
      for (auto& symbol : pair.mValue) {
         if constexpr (CT::Bool<R>) {
            if (not call(symbol))
               return counter;
         }
         else call(symbol);
         ++counter;
      }
   }

   return counter;
}
