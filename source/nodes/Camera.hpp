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
   ///   Camera node                                                          
   ///                                                                        
   struct Camera : Node {
   public:
      MaterialNodeCamera(MaterialNode*, const Verb&);
      MaterialNodeCamera(MaterialNodeCamera&&) noexcept = default;

      void Generate() final;

   private:
      void GenerateDefinition();
   };

} // namespace Nodes