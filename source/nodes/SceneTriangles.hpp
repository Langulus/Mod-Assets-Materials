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
   ///   Scene node                                                           
   ///                                                                        
   /// Can generate shader code that resermbles the described scene, either   
   /// by adding signed distance field functions, or triangles/lines          
   ///                                                                        
   struct SceneTriangles : Node {
      SceneTriangles(Node*, const Descriptor&);
      SceneTriangles(const Descriptor&);

      Symbol Generate() final;

      NOD() operator Debug() const;
   };

} // namespace Nodes