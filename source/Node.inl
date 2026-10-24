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
auto Node::GetMaterial() const noexcept -> Material* {
   return mMaterial;
}

/// Add a child                                                               
///   @attention assumes node is a valid pointer                              
///   @tparam TWOSIDED true to also set the node's parent                     
///      (used mainly internally to avoid endless loops)                      
///   @param node node instance to add as child                               
///   @return the number of added children                                    
template<bool TWOSIDED>
size_t Node::AddChild(Node* node) {
   LglsAssumeUser(node, "Bad node pointer");
   VERBOSE_NODE("Adding child ", node, " (", node->GetReferences(), " uses)");
   const auto added = mChildren.Merge(node);
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
///   @tparam TWOSIDED true to also remove node's owner                       
///      (used mainly internally to avoid endless loops)                      
///   @param node node instance to remove from children                       
///   @return the number of removed children                                  
template<bool TWOSIDED>
size_t Node::RemoveChild(Node* node) {
   LglsAssumeUser(node, "Bad node pointer");
   VERBOSE_NODE("Removing child ", node, " (", node->GetReferences(), " uses)");
   const auto removed = mChildren.Erase(node);
   if constexpr (TWOSIDED) {
      if (removed and node->mParent == this) {
         node->mParent = nullptr;
         node->mMaterial = nullptr;
      }
   }

   return removed;
}

/// Execute a function in each child node                                     
///   @param call the function callback                                       
///   @return the number of successul executions of the call                  
template<class F>
size_t Node::ForEachChild(F&& call) {
   auto counter = mChildren.ForEach(LglsFwd(call));
   for (auto child : mChildren)
      counter += child->ForEachChild(LglsFwd(call));
   return counter;
}

/// Add a local variable to the node, with a specific tag and initial value   
///   @tparam T tag to associate with the value                               
///   @param value the value itself                                           
///   @param variable optional name for the variable; if not name is given    
///      the value will be used as a literal constant                         
///   @return reference to the symbol that corresponds to the input           
template<CT::DefineTag T, CT::NotVoid D>
auto Node::AddLocal(D&& value, Token const& variable) -> Symbol const& {
   const auto meta = MetaTagOf<T>();
   mLocalsT[meta] << Symbol::Variable<T>(mRate, LglsFwd(value), variable);
   return mLocalsT[meta].Last();
}

/// Add a literal to the node, with a specific tag and value                  
///   @tparam T tag to associate with the value                               
///   @param value the value itself                                           
///   @return reference to the symbol that corresponds to the input           
template<CT::DefineTag T, CT::NotVoid D>
auto Node::AddLiteral(D&& value) -> Symbol const& {
   const auto meta = MetaTagOf<T>();
   mLocalsT[meta] << Symbol::Literal<T>(mRate, LglsFwd(value));
   return mLocalsT[meta].Last();
}

/// Add an output symbol to the node                                          
///   @tparam T the data type of the output (or return type of function)      
///   @param pattern the symbol name/function template                        
///   @param a... optional arguments, if symbol is a function template        
///      these arguments can be DMetas, or TMetas, or both by providing a Tag 
///      with a given type.                                                   
///   @return the symbol handle                                               
template<CT::NotVoid T>
auto Node::ExposeData(Token const& pattern, auto&&...a) -> Symbol& {
   const auto meta = MetaDataOf<T>();
   mLocalsD[meta] << Symbol::Function<T>(mRate, pattern, LglsFwd(a)...);
   return mLocalsD[meta].Last();
}

/// Add an output symbol to the node                                          
///   @tparam T the tag type of the output                                    
///   @tparam D the data type of the output (or return type of function)      
///   @param pattern the symbol name/function template                        
///   @param a... optional arguments, if symbol is a function template        
///      these arguments can be DMetas, or TMetas, or both by providing a Tag 
///      with a given type.                                                   
///   @return the symbol handle                                               
template<CT::DefineTag T, CT::NotVoid D>
auto Node::ExposeTag(Token const& pattern, auto&&...a) -> Symbol& {
   const auto meta = MetaDataOf<T>();
   mLocalsT[meta] << Symbol::Function<D>(mRate, pattern, LglsFwd(a)...);
   return mLocalsT[meta].Last();
}

/// Select a symbol from the node hierarchy.                                  
/// Checks local symbols first, then climbs up the hierarchy up to Root,      
/// where it starts selecting global material inputs/constants.               
///   @param t tag type filter                                                
///   @param d data type filter                                               
///   @param r rate filter                                                    
///   @param i index filter                                                   
///   @return a pointer to the symbol, or nullptr if not found                
template<class C>
auto Node::GetSymbol(this C&& self, TMeta t, DMeta d, RefreshRate r, CT::Index auto i) -> LglsMutIf(C, Symbol*) {
   size_t nth = i.IsSpecial() ? 0 : static_cast<size_t>(i.index);

   if (t) {
      // Filter by tags                                                 
      const auto foundt = self.mLocalsT.Find(t);
      if (foundt) {
         auto& candidates = self.mLocalsT.GetValue(foundt);
         if (i and i.IsSpecial()) {
            // Pick a special index                                     
            auto& candidate = candidates[i];
            if (candidate.MatchesFilter(d, r))
               return &candidate;
         }
         else {
            // Pick specific match from all candidates                  
            for (auto& candidate : candidates) {
               if (candidate.MatchesFilter(d, r)) {
                  if (0 == nth)
                     return &candidate;
                  --nth;
               }
            }
         }
      }
   }
   else if (d) {
      // Filter by data type                                            
      for (auto pair : self.mLocalsD) {
         if (not pair.GetKey()->CastsTo(d))
            continue;

         auto& candidates = pair.GetValue();
         if (i and i.IsSpecial()) {
            // Pick a special index                                     
            auto& candidate = candidates[i];
            if (candidate.MatchesFilter(d, r))
               return &candidate;
         }
         else {
            // Pick specific match from all candidates                  
            for (auto& candidate : candidates) {
               if (candidate.MatchesFilter(d, r)) {
                  if (0 == nth)
                     return &candidate;
                  --nth;
               }
            }
         }
      }
   }
   else if (r != Rate::Auto) {
      // Filter by rate                                                 
      TODO();
   }
   else if (i) {
      // Just return the nth (filter by index only)                     
      TODO();
   }

   // Nothing was found                                                 
   return nullptr;
}

/// Select a symbol from the node hierarchy                                   
/// Checks local symbols first, then climbs up the hierarchy up to Root,      
/// where it starts selecting global material inputs/constants                
///   @tparam T - tag type filter (use void to disable)                       
///   @tparam D - data type filter (use void to disable)                      
///   @param r - rate filter                                                  
///   @param i - index filter                                                 
///   @return a pointer to the symbol, or nullptr if not found                
template<class T, class D, class C>
auto Node::GetSymbol(this C&& self, RefreshRate r, int i) -> LglsMutIf(C, Symbol*) {
   static_assert(CT::Void<T> or CT::Tag<T>,
      "T must be either tag, or void");
   static_assert(CT::Void<D> or CT::Data<D>,
      "D must be either data type, or void");

   return GetSymbol(MetaOf<T>(), MetaOf<D>(), r, i);
}

/// Get the refresh rate of the node                                          
LANGULUS(INLINED)
auto Node::GetRate() const noexcept -> RefreshRate {
   return mRate;
}

/// Execute a function for each symbol in the inputs                          
///   @tparam F - function signature (deducible)                              
///   @param call - the function to call for each symbol                      
///   @return the number of execution of call                                 
template<class F>
size_t Node::ForEachInput(F&& call) {
   using A = ArgumentOf<F>;
   using R = ReturnOf<F>;
   static_assert(Same<A, Symbol>, "Function argument must be a Symbol");

   size_t counter = 0;
   for (auto pair : mLocalsT) {
      for (auto& symbol : pair.GetValue()) {
         if constexpr (CT::Bool<R>) {
            if (not call(symbol))
               return counter;
         }
         else call(symbol);
         ++counter;
      }
   }

   for (auto pair : mLocalsD) {
      for (auto& symbol : pair.GetValue()) {
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
size_t Node::ForEachOutput(F&& call) {
   using A = ArgumentOf<F>;
   using R = ReturnOf<F>;
   static_assert(CT::Same<A, Symbol>, "Function argument must be a Symbol");

   size_t counter {};
   for (auto pair : mOutputsT) {
      for (auto& symbol : pair.GetValue()) {
         if constexpr (CT::Bool<R>) {
            if (not call(symbol))
               return counter;
         }
         else call(symbol);
         ++counter;
      }
   }

   for (auto pair : mOutputsD) {
      for (auto& symbol : pair.GetValue()) {
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
