///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
///                                                                           
#pragma once
#include "Symbol.hpp"
#include <Langulus/Verbs/Add.hpp>
#include <Langulus/Verbs/Multiply.hpp>
#include <Langulus/Verbs/Modulate.hpp>
#include <Langulus/Verbs/Exponent.hpp>
#include <Langulus/Verbs/Randomize.hpp>


///                                                                           
///   Abstract material node                                                  
///                                                                           
struct Node : Part {
protected:
   friend struct Material;
   friend struct Nodes::FBM;

   // The children that this node leads to                              
   TMany<Ref<Node>> mChildren;

   // The material this node belongs to                                 
   Material* mMaterial {};

   // The rate at which this node is refreshed                          
   RefreshRate mRate = Rate::Auto;

   // Local variables, usually used by selection verbs, when executing  
   // Code in the context of the Node                                   
   TUnorderedMap<TMeta, Symbols> mLocalsT;
   TUnorderedMap<DMeta, Symbols> mLocalsD;

   // The outputs this Node exposes                                     
   // They can be Selected from the hierarchy                           
   TUnorderedMap<TMeta, Symbols> mOutputsT;
   TUnorderedMap<DMeta, Symbols> mOutputsD;

   // Whether or not this node's code has already been generated        
   // Protects against infinite dependency loops                        
   bool mGenerated = false;

   // The parents that lead to this node                                
   Ref<Node> mParent;

   // The normalized descriptor                                         
   Neat mDescriptor;

   struct DefaultTrait {
      DMeta mType;
      RefreshRate mRate;
   };

   static inline const Symbol NoSymbol {};

public:
   using CTTI_Producer = Node;
   LANGULUS_VERBS(
      Verbs::Create,
      Verbs::Select,
      Verbs::Add,
      Verbs::Multiply,
      Verbs::Modulate,
      Verbs::Exponent,
      Verbs::Randomize
   );

   Node(Material*, Many const&);
   Node(Node*, Many const&);
   Node(Many const&);
   Node(Node&&) = delete;

   virtual ~Node();
   virtual void Detach();

   void Create(Verb&);
   void Select(Verb&);
   void Add(Verb&);
   void Multiply(Verb&);
   void Modulate(Verb&);
   void Exponent(Verb&);
   void Randomize(Verb&);

   void Dump() const;

   virtual const Symbol& Generate() = 0;

   auto GetRate() const noexcept -> RefreshRate;
   auto GetStage() const -> size_t;
   auto GetMaterial() const noexcept -> Material*;
   auto GetLibrary() const noexcept -> MaterialLibrary*;
   static auto GetDefaultTrait(TMeta) -> DefaultTrait;
   static auto DecayToGLSLType(DMeta) -> DMeta;

   template<bool TWOSIDED = true>
   size_t AddChild(Node*);
   template<bool TWOSIDED = true>
   size_t RemoveChild(Node*);

   void Descend();

   template<class F>
   size_t ForEachChild(F&&);

   auto GetSymbol(TMeta, DMeta = nullptr, RefreshRate = Rate::Auto, Index = IndexLast)       -> Symbol*;
   auto GetSymbol(TMeta, DMeta = nullptr, RefreshRate = Rate::Auto, Index = IndexLast) const -> Symbol const*;

   template<class = void, class = void>
   auto GetSymbol(RefreshRate = Rate::Auto, Index = IndexLast) -> Symbol*;
   template<class = void, class = void>
   auto GetSymbol(RefreshRate = Rate::Auto, Index = IndexLast) const -> Symbol const*;

   template<class F>
   size_t ForEachInput(F&&);
   template<class F>
   size_t ForEachOutput(F&&);
   
protected:
   void InnerCreate();
   auto NodeFromConstruct(Recipe const&) -> Node*;

   Text DebugBegin() const;
   Text DebugEnd() const;

   template<CT::Tag T, CT::NotVoid D>
   auto AddLocal(D&&, Token const&) -> const Symbol&;
   
   template<CT::Tag T, CT::NotVoid D>
   auto AddLiteral(D&&) -> const Symbol&;

   template<CT::NotVoid T, class... ARGS>
   auto ExposeData(Token const&, ARGS&&...) -> Symbol&;

   template<CT::Tag T, CT::NotVoid D, class... ARGS>
   auto ExposeTrait(Token const&, ARGS&&...) -> Symbol&;

   void AddDefine(Token const&, const GLSL&);

   void ArithmeticVerb(Verb&, Token const& pos, Token const& neg = {}, Token const& una = {});
};

#include "Node.inl"
