///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include <Entity/External.hpp>

using namespace Langulus;
using namespace Langulus::Flow;
using namespace Langulus::Anyness;
using namespace Langulus::Entity;
using namespace Langulus::Math;

struct GUI;
struct GUISystem;
struct GUIItem;
struct GUIFont;

/// Include ImGui                                                             
#include <imgui.h>

#if LANGULUS(DEBUG)
   #define IMGUI_VULKAN_DEBUG_REPORT
#endif