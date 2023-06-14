///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "GLSL.hpp"
#include <Flow/Rate.hpp>


///                                                                           
///   A shader code symbol                                                    
///                                                                           
/// Transfers data from one node to another. Can be an expression, a literal, 
/// a name to a variable, or a template to a function call                    
///                                                                           
struct Symbol {
   // The rate at which this symbol is refreshed                        
   Rate mRate {Rate::Auto};
   // The trait (if any), the data type (if any), and the value (if     
   // the symbol is a constant/literal). If this symbol represents a    
   // function call, then this is its return type                       
   Trait mTrait;
   // The generated code for the symbol. Will contain a template, if    
   // this symbol is for a function call                                
   GLSL mCode;
   // Number of elements, if symbold is an array                        
   Count mCount {1};
   // List of arguments, in case this symbol is a function call template
   // One must TemplateFill mCode with these traits to instantiate the  
   // symbol                                                            
   TAny<Trait> mArguments;
   // Number of times a symbol is used                                  
   // If an expression with many uses, the symbol will be moved to a    
   // variable, that will be used instead                               
   Count mUses {1};

   template<CT::Data T, class... ARGS>
   NOD() static Symbol Function(Rate, const Token&, ARGS&&...);

protected:
   void PushArgument(DMeta&&);
   void PushArgument(TMeta&&);
   void PushArgument(Trait&&);
};

using Symbols = TAny<Symbol>;

#include "Symbol.inl"