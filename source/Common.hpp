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

struct MaterialLibrary;
struct Material;
struct Node;

LANGULUS_EXCEPTION(Material);

#define VERBOSE_NODE(...) Logger::Verbose(Self(), __VA_ARGS__)

/// Fill template arguments using libfmt                                      
///   @tparam ...ARGS - arguments for the template                            
///   @param format - the template string                                     
///   @param args... - the arguments                                          
///   @return the instantiated template                                       
template<class... ARGS>
Text TemplateFill(const Token& format, ARGS&&...args) {
   const auto size = fmt::formatted_size(
      fmt::format_string<ARGS...> {format},
      Forward<ARGS>(args)...
   );

   Text result;
   result.Reserve(size);
   fmt::format_to_n(result.GetRaw(), size, format, Forward<ARGS>(args)...);
   return Abandon(result);
}

namespace Nodes::Inner
{
   template<::std::size_t...N>
   constexpr auto CheckPattern(const Token& pattern, ::std::index_sequence<N...>) {
      return fmt::format_string<decltype(N)...> {pattern};
   }
}

/// Attempt filling the template, statically checking if argument count is    
/// satisfied, and other erroneous conditions. Since arguments contain        
/// unformattable data, that has more to do with seeking actual data from the 
/// node hierarchy later, arguments are substituted with an index sequence.   
///   @tparam ...ARGS - arguments for the template                            
///   @param format - the template string                                     
///   @param args... - the arguments                                          
template<class... ARGS>
constexpr auto TemplateCheck(const Token& format, ARGS&&...args) {
   return Nodes::Inner::CheckPattern(format, ::std::make_index_sequence<sizeof...(ARGS)> {});
}

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