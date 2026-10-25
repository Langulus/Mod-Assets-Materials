///                                                                           
/// Langulus::Module::Art::Materials                                          
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
///                                                                           
#include "Light.hpp"

using namespace Nodes;


/// Light node creation                                                       
///   @param desc descriptor for the node                                     
Light::Light(Describe&& descriptor)
   : Downcastable {this}
   , Node         {*descriptor} { }

/// Generate the light functionality                                          
///   @return the light function template                                     
Symbol const& Light::Generate() {
   // Generate children first, if any                                   
   Descend();
   TODO();
   return NoSymbol;
}
