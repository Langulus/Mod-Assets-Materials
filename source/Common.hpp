///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
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

#define VERBOSE_NODE(...)     Logger::Verbose(Self(), __VA_ARGS__)
#define VERBOSE_NODE_TAB(...) const auto tab = Logger::Verbose(Self(), __VA_ARGS__, Logger::Tabs {})


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