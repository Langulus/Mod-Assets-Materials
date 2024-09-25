///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
///                                                                           
#pragma once
#include "Symbol.hpp"
#include <Math/Verbs/Add.hpp>
#include <Math/Verbs/Multiply.hpp>
#include <Math/Verbs/Modulate.hpp>
#include <Math/Verbs/Exponent.hpp>
#include <Math/Verbs/Randomize.hpp>


///                                                                           
///   Abstract material node                                                  
///                                                                           
struct Node : A::Unit {
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
   LANGULUS(PRODUCER) Node;
   LANGULUS_VERBS(
      Verbs::Create,
      Verbs::Select,
      Verbs::Add,
      Verbs::Multiply,
      Verbs::Modulate,
      Verbs::Exponent,
      Verbs::Randomize
   );

   Node(Material*, Describe);
   Node(Node*, Describe);
   Node(Describe);
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

   NOD() RefreshRate GetRate() const noexcept;
   NOD() Offset GetStage() const;
   NOD() Material* GetMaterial() const noexcept;
   NOD() MaterialLibrary* GetLibrary() const noexcept;
   NOD() static DefaultTrait GetDefaultTrait(TMeta);
   NOD() static DMeta DecayToGLSLType(DMeta);

   template<bool TWOSIDED = true>
   Count AddChild(Node*);
   template<bool TWOSIDED = true>
   Count RemoveChild(Node*);

   void Descend();

   template<class F>
   Count ForEachChild(F&&);

   NOD() Symbol*        GetSymbol(TMeta, DMeta = nullptr, RefreshRate = Rate::Auto, Index = IndexLast);
   NOD() Symbol const*  GetSymbol(TMeta, DMeta = nullptr, RefreshRate = Rate::Auto, Index = IndexLast) const;

   template<class = void, class = void>
   NOD() Symbol*        GetSymbol(RefreshRate = Rate::Auto, Index = IndexLast);
   template<class = void, class = void>
   NOD() Symbol const*  GetSymbol(RefreshRate = Rate::Auto, Index = IndexLast) const;

   template<class F>
   Count ForEachInput(F&&);
   template<class F>
   Count ForEachOutput(F&&);
   
protected:
   void InnerCreate();
   Node* NodeFromConstruct(const Construct&);

   NOD() Text DebugBegin() const;
   NOD() Text DebugEnd() const;

   template<CT::Trait T, CT::Data D>
   const Symbol& AddLocal(D&&, const Token&);
   
   template<CT::Trait T, CT::Data D>
   const Symbol& AddLiteral(D&&);

   template<CT::Data T, class... ARGS>
   Symbol& ExposeData(const Token&, ARGS&&...);

   template<CT::Trait T, CT::Data D, class... ARGS>
   Symbol& ExposeTrait(const Token&, ARGS&&...);

   void AddDefine(const Token&, const GLSL&);

   void ArithmeticVerb(Verb&, const Token& pos, const Token& neg = {}, const Token& una = {});
};

#include "Node.inl"
