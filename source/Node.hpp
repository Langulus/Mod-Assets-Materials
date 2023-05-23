///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "GLSL.hpp"

/// Transformers                                                              
/*PC_DECLARE_VERB(Project, Unproject, 
   "Project/unproject stuff, depending on camera orientation or other data");

/// Fractal Brownian motion                                                   
PC_DECLARE_VERB(FBM, FBM,
   "Fractal Brownian motion generator");

/// Scene renderers                                                           
PC_DECLARE_VERB(Raymarch, Raymarch, 
   "Raymarch, integrating light across the scene");
PC_DECLARE_VERB(Rasterize, Rasterize, 
   "Rasterize, projecting primitives to screen and shading them");
PC_DECLARE_VERB(Raycast, Raycast, 
   "Raycast, intersecting the scene and colorizing it accordingly");
PC_DECLARE_VERB(Raytrace, Raytrace, 
   "Raytrace, following light trajectory, reflections and volumes throughout the scene");

/// Colorizers                                                                
PC_DECLARE_VERB(Illuminate, Darken, 
   "Perform light calculation based on normals causing ambient, diffuse and specular highlights");
PC_DECLARE_VERB(Texturize, Texturize,
   "Apply a texture to a surface or volume - solid colors, color maps, normal maps, displacement maps, etc...");

/// Useful constants                                                          
LANGULUS_DECLARE_CONSTANT(FrontFace, Trait::From<Traits::Texture>(0),
   "Used as texture ID, dedicated to front polygonal faces");
LANGULUS_DECLARE_CONSTANT(BackFace, Trait::From<Traits::Texture>(1),
   "Used as texture ID, dedicated to back polygonal faces");*/


///                                                                           
///   Abstract material node                                                  
///                                                                           
class Node {
protected:
   friend struct Material;

   enum ValueType { Input, Output };
   ValueType mType = ValueType::Input;

   // The rate at which this node is refreshed                          
   Rate mRate = RRate::Error;

   // The node constructor                                              
   Construct mConstruct;

   // The parents that lead to this node                                
   Ptr<Node> mParent;

   // The children that this node leads to                              
   TAny<Node*> mChildren;

   // The input traits that this node consumes                          
   TMap<Trait, GLSL> mInputs;

   // The output traits that this node produces                         
   TMap<Trait, GLSL> mOutputs;

   // Whether or not this node's code has already been generated        
   // Protects against infinite dependency loops                        
   bool mGenerated = false;

public:
   Node() = delete;
   Node(DMeta, Material*);
   Node(DMeta, Node*, const Verb& = {});

   PC_VERB(Create);
   PC_VERB(Select);

   NOD() operator Debug() const;
   NOD() operator GLSL() const;

   virtual void Generate() = 0;

   /// Get node hex ID based on its pointer                                   
   NOD() inline GLSL GetNodeID() const noexcept {
      return pcToHex(this);
   }

   NOD() MContent* GetContentManager() const noexcept;
   NOD() const TAny<Entity*>& GetOwners() const noexcept;
   NOD() static RRate Rate(const Verb&, RRate);

   NOD() const Trait& GetOutputTrait() const;
   NOD() Trait GetOutputTrait(const Trait&) const;
   NOD() const GLSL& GetOutputSymbol() const;
   NOD() GLSL GetOutputSymbol(const Trait&) const;
   NOD() GLSL GetOutputSymbolAs(DMeta, real) const;
   NOD() GLSL GetOutputSymbolAs(const Trait&, DMeta, real) const;

   NOD() MaterialNodeValue GetValue(TMeta, DMeta, RRate, bool = true);

   NOD() GLSL GetSymbol(TMeta, DMeta, RRate, bool = true);

   NOD() static DMeta DefaultTraitType(TMeta);
   NOD() static RRate DefaultTraitRate(TMeta);
   NOD() static DMeta DecayToGLSLType(DMeta);

   /// Get input symbol (with modifications if any)                           
   ///   @param rate - the rate of the trait to use                           
   ///   @param addIfMissing - add an input automatically, if missing         
   ///   @return the symbol and usage                                         
   template<RTTI::ReflectedTrait TRAIT, RTTI::ReflectedData DATA = void>
   NOD() MaterialNodeValue GetValue(RRate rate = RRate::PerAuto, bool addIfMissing = true) {
      return GetValue(TRAIT::Reflect(), DataID::Reflect<DATA>(), rate, addIfMissing);
   }
   template<RTTI::ReflectedTrait TRAIT, RTTI::ReflectedData DATA = void>
   NOD() GLSL GetSymbol(RRate rate = RRate::PerAuto, bool addIfMissing = true) {
      return GetSymbol(TRAIT::Reflect(), DataID::Reflect<DATA>(), rate, addIfMissing);
   }

   void AddChild(MaterialNode*);
   void RemoveChild(MaterialNode*);

   template<class F>
   pcptr ForEachChild(F&& call) {
      auto counter = mChildren.ForEach(pcForward<F>(call));
      for (auto child : mChildren)
         counter += child->ForEachChild(pcForward<F>(call));
      return counter;
   }

   template<class T>
   T* FindChild() {
      for (auto child : mChildren) {
         auto asT = dynamic_cast<T*>(child);
         if (asT)
            return asT;
      }
      return nullptr;
   }

private:
   NOD() bool InnerGetValue(const Trait&, RRate, bool, MaterialNodeValue&) const;
   bool IsInHierarchy(MaterialNode*) const;

public:
   /// Add an output connection to this node                                  
   ///   @param constructor - the instance constructor                        
   ///   @return a pointer to the child                                       
   template<RTTI::ReflectedTrait TRAIT, RTTI::ReflectedData DATA>
   void Expose(const GLSL& code) {
      mOutputs.Add(Trait::From<TRAIT, DATA>(), code);
   }

   /// Add an output connection to this node                                  
   ///   @param constructor - the instance constructor                        
   ///   @return a pointer to the child                                       
   template<class T>
   NOD() T* EmplaceChild(T&& constructor) {
      const auto newNode = new T {pcForward<T>(constructor)};
      AddChild(newNode);
      return static_cast<T*>(mChildren.Last());
   }

   /// Add an output connection to this node, reusing a node of same type     
   ///   @param constructor - the instance constructor                        
   ///   @return a pointer to the child                                       
   template<class T>
   NOD() T* EmplaceChildUnique(T&& constructor) {
      T* result = nullptr;
      mChildren.ForEach([&](T* node) { result = node; });
      return result ? result : EmplaceChild<T>(pcForward<T>(constructor));
   }

   /// Get the refresh rate of the node                                       
   ///   @return the rate of this node                                        
   NOD() inline RRate GetRate() const noexcept {
      return mRate;
   }

   /// Consume the node (mark as generated)                                   
   inline void Consume() noexcept {
      mGenerated = true;
   }

   /// Check if this node's code is already generated                         
   NOD() inline bool IsConsumed() const noexcept {
      return mGenerated;
   }

   /// Check if this node's code is already generated                         
   NOD() inline auto& GetOutputs() const noexcept {
      return mOutputs;
   }

   /// Get stage from node rate                                               
   NOD() ShaderStage::Enum GetStage() const;

   void Descend();
   void Dump() const;
   void Commit(ShaderToken::Enum, const GLSL&);

   NOD() GLSL CodeFromConstruct(const Construct&);
   NOD() GLSL CodeFromScope(const Any&);

   NOD() inline bool IsInput() const {
      return mType == ValueType::Input;
   }

protected:
   NOD() Debug DebugBegin() const;
   NOD() Debug DebugEnd() const;
};

/// Check if keyframe is relative, by searching for corresponding trait       
///   @param keyframe - the keyframe to analyze                               
///   @return true if keyframe is relative                                    
NOD() inline bool IsRelativeKeyframe(const Verb& keyframe) {
   bool relative = false;
   keyframe.GetArgument().ForEachDeep([&](const Block& group) {
      group.ForEach([&](const Trait& trait) {
         if (trait.TraitIs<Traits::Relative>())
            relative = trait.AsCast<bool>();
      });
   });

   return relative;
}
