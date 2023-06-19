///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "Symbol.hpp"
#include <Flow/Verbs/Create.hpp>
#include <Flow/Verbs/Select.hpp>
#include <Flow/Verbs/Add.hpp>
#include <Flow/Verbs/Multiply.hpp>
#include <Flow/Verbs/Modulate.hpp>
#include <Flow/Verbs/Exponent.hpp>
#include <Flow/Verbs/Randomize.hpp>


///                                                                           
///   Abstract material node                                                  
///                                                                           
struct Node : Unit {
protected:
   friend struct Material;
   friend struct Nodes::FBM;

   // The normalized descriptor                                         
   Normalized mDescriptor;
   // The material this node belongs to                                 
   Material* mMaterial {};

   // The rate at which this node is refreshed                          
   Rate mRate {Rate::Auto};
   // The parents that lead to this node                                
   Ref<Node> mParent;
   // The children that this node leads to                              
   TAny<Node*> mChildren;

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

   struct DefaultTrait {
      DMeta mType;
      Rate  mRate;
   };

public:
   LANGULUS_VERBS(
      Verbs::Create,
      Verbs::Select,
      Verbs::Add,
      Verbs::Multiply,
      Verbs::Modulate,
      Verbs::Exponent,
      Verbs::Randomize
   );

   Node(DMeta, Material*, const Descriptor&);
   Node(DMeta, Node*, const Descriptor&);
   Node(DMeta, const Descriptor&);

   void Create(Verb&);
   void Select(Verb&);
   void Add(Verb&);
   void Multiply(Verb&);
   void Modulate(Verb&);
   void Exponent(Verb&);
   void Randomize(Verb&);

   NOD() operator Debug() const;
   void Dump() const;

   virtual Symbol& Generate() = 0;

   NOD() Rate GetRate() const noexcept;
   NOD() Offset GetStage() const;
   NOD() Material* GetMaterial() const noexcept;
   NOD() MaterialLibrary* GetLibrary() const noexcept;
   NOD() static const DefaultTrait& GetDefaultTrait(TMeta);
   NOD() static DMeta DecayToGLSLType(DMeta);

   void AddChild(Node*);
   void RemoveChild(Node*);
   void Descend();

   template<class F>
   Count ForEachChild(F&&);

   template<CT::Data T, class... ARGS>
   Symbol& Expose(const Token&, ARGS&&...);

   NOD()       Symbol* GetSymbol(TMeta, DMeta = nullptr, Rate = Rate::Auto, Index = IndexLast);
   NOD() const Symbol* GetSymbol(TMeta, DMeta = nullptr, Rate = Rate::Auto, Index = IndexLast) const;

   template<class = void, class = void>
   NOD()       Symbol* GetSymbol(Rate = Rate::Auto, Index = IndexLast);
   template<class = void, class = void>
   NOD() const Symbol* GetSymbol(Rate = Rate::Auto, Index = IndexLast) const;

   template<class F>
   Count ForEachInput(F&&);
   template<class F>
   Count ForEachOutput(F&&);

   //NOD() const Hierarchy& GetOwners() const noexcept;

   /*NOD() const Trait& GetOutputTrait() const;
   NOD() Trait GetOutputTrait(const Trait&) const;
   NOD() const GLSL& GetOutputSymbol() const;
   NOD() GLSL GetOutputSymbol(const Trait&) const;
   NOD() GLSL GetOutputSymbolAs(DMeta, Real) const;
   NOD() GLSL GetOutputSymbolAs(const Trait&, DMeta, Real) const;

   NOD() Nodes::Value GetValue(TMeta, DMeta, Rate, bool = true);

   NOD() GLSL GetSymbol(TMeta, DMeta, Rate, bool = true);*/


   /*template<CT::Trait, class = void>
   NOD() Nodes::Value GetValue(Rate = Rate::Auto, bool addIfMissing = true);

   template<CT::Trait, class = void>
   NOD() GLSL GetSymbol(Rate = Rate::Auto, bool addIfMissing = true);*/



   //template<class T>
   //T* FindChild();

   //template<CT::Trait, CT::Data>
   //void Expose(const GLSL&);


   /*template<class T>
   NOD() T* EmplaceChild(T&&);
   template<class T>
   NOD() T* EmplaceChildUnique(T&&);*/

   //NOD() bool IsConsumed() const noexcept;
   //NOD() auto& GetOutputs() const noexcept;

   //void Consume() noexcept;
   //void Commit(const Token&, const GLSL&);

   /*NOD() GLSL CodeFromConstruct(const Construct&);
   NOD() GLSL CodeFromScope(const Any&);
   NOD() bool IsInput() const;*/

protected:
   void InnerCreate();
   Node* NodeFromConstruct(const Construct&);

   /*NOD() bool InnerGetValue(const Trait&, Rate, bool, Nodes::Value&) const;
   bool IsInHierarchy(Node*) const;*/

   NOD() Debug DebugBegin() const;
   NOD() Debug DebugEnd() const;

   GLSL AddInput(const Trait&, bool allowDuplicates);
   GLSL AddOutput(const Trait&, bool allowDuplicates);
   GLSL AddDefine(const Token&, const GLSL&);

   void ArithmeticVerb(Verb&, const Token& pos, const Token& neg = {}, const Token& una = {});
};

//NOD() bool IsRelativeKeyframe(const Verb&);

#include "Node.inl"
