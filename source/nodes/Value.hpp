///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "../Node.hpp"


namespace Nodes
{

   ///                                                                        
   ///   Value node                                                           
   ///                                                                        
   struct Value : Node {
   private:
      Trait mTrait;
      GLSL mUniform, mName, mUse;

   public:
      LANGULUS_VERBS(Verbs::Do);

      Value(const Descriptor&);

      NOD() static Value Input (Node*, const Trait& = {}, Rate = Rate::Auto, const GLSL& name = {});
      NOD() static Value Output(Node*, const Trait& = {}, Rate = Rate::Auto, const GLSL& name = {});
      NOD() static Value Local (Node*, const Trait& = {}, Rate = Rate::Auto, const GLSL& name = {});

      Symbol Generate() final;

      void Do(Verb&);

      NOD() GLSL SelectMember(TMeta, Trait&);
      NOD() GLSL GetDeclaration() const;
      NOD() TMeta GetTrait() const noexcept;

      void BindTo(const Trait&, const Node*);
      bool IsValid() const;

      NOD() operator Debug() const;

   private:
      void AutoCompleteTrait();
   };

} // namespace Nodes