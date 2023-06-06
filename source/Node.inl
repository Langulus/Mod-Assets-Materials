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
Material* Node::GetMaterial() const noexcept {
   return mMaterial;
}

/// Get input symbol (with modifications if any)                              
///   @param rate - the rate of the trait to use                              
///   @param addIfMissing - add an input automatically, if missing            
///   @return the symbol and usage                                            
template<CT::Trait T, class D>
LANGULUS(INLINED)
Nodes::Value Node::GetValue(Rate rate, bool addIfMissing) {
   return GetValue(
      RTTI::MetaTrait::Of<T>(), 
      RTTI::MetaData::Of<D>(),
      rate, addIfMissing
   );
}

/// Get input symbol (with modifications if any)                              
///   @param rate - the rate of the trait to use                              
///   @param addIfMissing - add an input automatically, if missing            
///   @return the symbol and usage                                            
template<CT::Trait T, class D>
LANGULUS(INLINED)
GLSL Node::GetSymbol(Rate rate, bool addIfMissing) {
   return GetSymbol(
      RTTI::MetaTrait::Of<T>(), 
      RTTI::MetaData::Of<D>(),
      rate, addIfMissing
   );
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

/// Check if a child of specific type exists                                  
template<class T>
LANGULUS(INLINED)
T* Node::FindChild() {
   for (auto child : mChildren) {
      auto asT = dynamic_cast<T*>(child);
      if (asT)
         return asT;
   }
   return nullptr;
}

/// Add an output connection to this node                                     
///   @param constructor - the instance constructor                           
///   @return a pointer to the child                                          
template<CT::Trait T, CT::Data D>
LANGULUS(INLINED)
void Node::Expose(const GLSL& code) {
   mOutputs.Insert(Trait::From<T, D>(), code);
}

/// Add an output connection to this node                                     
///   @param constructor - the instance constructor                           
///   @return a pointer to the child                                          
template<class T>
LANGULUS(INLINED)
T* Node::EmplaceChild(T&& constructor) {
   const auto newNode = new T {Forward<T>(constructor)};
   AddChild(newNode);
   return static_cast<T*>(mChildren.Last());
}

/// Add an output connection to this node, reusing a node of same type        
///   @param constructor - the instance constructor                           
///   @return a pointer to the child                                          
template<class T>
LANGULUS(INLINED)
T* Node::EmplaceChildUnique(T&& constructor) {
   T* result = nullptr;
   mChildren.ForEach([&](T* node) {
      result = node;
   });
   return result ? result : EmplaceChild<T>(Forward<T>(constructor));
}

/// Get the refresh rate of the node                                          
///   @return the rate of this node                                           
LANGULUS(INLINED)
Rate Node::GetRate() const noexcept {
   return mRate;
}

/// Consume the node (mark as generated)                                      
LANGULUS(INLINED)
void Node::Consume() noexcept {
   mGenerated = true;
}

/// Check if this node's code is already generated                            
LANGULUS(INLINED)
bool Node::IsConsumed() const noexcept {
   return mGenerated;
}

/// Check if this node's code is already generated                            
LANGULUS(INLINED)
auto& Node::GetOutputs() const noexcept {
   return mOutputs;
}

LANGULUS(INLINED)
bool Node::IsInput() const {
   return mType == ValueType::Input;
}

/// Check if keyframe is relative, by searching for corresponding trait       
///   @param keyframe - the keyframe to analyze                               
///   @return true if keyframe is relative                                    
LANGULUS(INLINED)
bool IsRelativeKeyframe(const Verb& keyframe) {
   bool relative = false;
   keyframe.ForEachDeep([&](const Block& group) {
      group.ForEach([&](const Trait& trait) {
         if (trait.TraitIs<Traits::Relative>())
            relative = trait.AsCast<bool>();
      });
   });

   return relative;
}
