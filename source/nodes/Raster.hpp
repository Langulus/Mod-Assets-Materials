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
   ///   Rasterizer material node                                             
   ///                                                                        
   struct Raster : Node {
   private:
      // Code for the rasterizer                                        
      Code mCode;
      // Whether or not to rasterize both sides of triangles            
      bool mBilateral {};
      // Whether or not triangle faces are flipped                      
      bool mSigned {};
      // Whether we're rasterizing triangles or lines                   
      DMeta mTopology {};
      // The depth range in which we're rasterizing                     
      Range1 mDepth {0, 1000};

   public:
      Raster(const Descriptor&);

      Symbol Generate() final;

   private:
      void GeneratePerPixel();
      void GeneratePerVertex();
   };

} // namespace Nodes