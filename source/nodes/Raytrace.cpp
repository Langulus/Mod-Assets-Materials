///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
///                                                                           
#include "Raytrace.hpp"

using namespace Nodes;


/// Raytracer node descriptor-constructor                                     
///   @param desc - raytrace descriptor                                       
Raytrace::Raytrace(Describe&& descriptor)
   : Resolvable {this}
   , Node {*descriptor} { }

/// Generate raytracer code                                                   
///   @return the output symbol                                               
const Symbol& Raytrace::Generate() {
   Descend();
   TODO();
   return NoSymbol;
}
