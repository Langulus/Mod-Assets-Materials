///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include <Langulus.hpp>
#include <Langulus/Material.hpp>

using namespace Langulus;
using namespace Math;

struct MaterialLibrary;
struct Material;
struct Node;

#if 0
   #define VERBOSE_NODE(...)     Logger::Verbose(Self(), __VA_ARGS__)
   #define VERBOSE_NODE_TAB(...) const auto tab = Logger::VerboseTab(Self(), __VA_ARGS__)
#else
   #define VERBOSE_NODE(...)     LANGULUS(NOOP)
   #define VERBOSE_NODE_TAB(...) LANGULUS(NOOP)
#endif


namespace Nodes
{
   struct Root;
   struct Camera;
   struct FBM;
   struct Light;
   struct Raster;
   struct Raycast;
   struct Raymarch;
   struct Raytrace;
   struct Scene;
   struct Texture;
   struct Transform;
   struct Value;
}