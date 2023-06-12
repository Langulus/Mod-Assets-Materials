///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "Symbol.hpp"


///                                                                           
///   Abstract material node                                                  
///                                                                           
struct Node : Unit {
protected:
   friend struct Material;
   friend struct Nodes::FBM;

   enum ValueType {Input, Output};

   // The normalized descriptor                                         
   Normalized mDescriptor;
   // The material this node belongs to                                 
   Material* mMaterial {};

   // Type of the node                                                  
   ValueType mType {ValueType::Input};
   // The rate at which this node is refreshed                          
   Rate mRate {Rate::Auto};
   // The parents that lead to this node                                
   Ref<Node> mParent;
   // The children that this node leads to                              
   TAny<Node*> mChildren;

   // The input traits that this node consumes                          
   TUnorderedMap<Trait, GLSL> mInputs;
   // The output traits that this node produces                         
   TUnorderedMap<Trait, GLSL> mOutputs;

   // Whether or not this node's code has already been generated        
   // Protects against infinite dependency loops                        
   bool mGenerated = false;


   struct DefaultTrait {
      DMeta mType;
      Rate  mRate;
   };

public:
   LANGULUS_VERBS(Verbs::Create, Verbs::Select);

   Node(DMeta, Material*, const Descriptor&);
   Node(DMeta, Node*, const Descriptor&);
   Node(DMeta, const Descriptor&);

   void Create(Verb&);
   void Select(Verb&);

   NOD() operator Debug() const;
   NOD() operator GLSL() const;

   virtual Symbol Generate() = 0;

   NOD() Material* GetMaterial() const noexcept;
   NOD() MaterialLibrary* GetLibrary() const noexcept;
   NOD() const Hierarchy& GetOwners() const noexcept;

   NOD() const Trait& GetOutputTrait() const;
   NOD() Trait GetOutputTrait(const Trait&) const;
   NOD() const GLSL& GetOutputSymbol() const;
   NOD() GLSL GetOutputSymbol(const Trait&) const;
   NOD() GLSL GetOutputSymbolAs(DMeta, Real) const;
   NOD() GLSL GetOutputSymbolAs(const Trait&, DMeta, Real) const;

   NOD() Nodes::Value GetValue(TMeta, DMeta, Rate, bool = true);

   NOD() GLSL GetSymbol(TMeta, DMeta, Rate, bool = true);

   NOD() static const DefaultTrait& GetDefaultTrait(TMeta);
   NOD() static DMeta DecayToGLSLType(DMeta);

   template<CT::Trait, class = void>
   NOD() Nodes::Value GetValue(Rate = PerAuto, bool addIfMissing = true);

   template<CT::Trait, class = void>
   NOD() GLSL GetSymbol(Rate = PerAuto, bool addIfMissing = true);

   void AddChild(Node*);
   void RemoveChild(Node*);

   template<class F>
   Count ForEachChild(F&&);

   template<class T>
   T* FindChild();

   template<CT::Trait, CT::Data>
   void Expose(const GLSL&);

   template<class T>
   NOD() T* EmplaceChild(T&&);
   template<class T>
   NOD() T* EmplaceChildUnique(T&&);

   NOD() Rate GetRate() const noexcept;
   NOD() bool IsConsumed() const noexcept;
   NOD() auto& GetOutputs() const noexcept;
   NOD() Offset GetStage() const;

   void Consume() noexcept;
   void Descend();
   void Dump() const;
   void Commit(const Token&, const GLSL&);

   NOD() GLSL CodeFromConstruct(const Construct&);
   NOD() GLSL CodeFromScope(const Any&);
   NOD() bool IsInput() const;

protected:
   void InnerCreate();
   Node* NodeFromConstruct(const Construct&);

   NOD() bool InnerGetValue(const Trait&, Rate, bool, Nodes::Value&) const;
   bool IsInHierarchy(Node*) const;

   NOD() Debug DebugBegin() const;
   NOD() Debug DebugEnd() const;

   GLSL AddDefine(const Token&, const GLSL&);
};

NOD() bool IsRelativeKeyframe(const Verb&);

#include "Node.inl"