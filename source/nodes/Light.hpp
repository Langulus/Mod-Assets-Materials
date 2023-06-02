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
   ///   Light material node                                                  
   ///                                                                        
   struct Light : Node {
      Light(const Descriptor&);

      void Generate() final;

   private:
      void GenerateDefinition();
      void GenerateUsage();
   };

} // namespace Nodes