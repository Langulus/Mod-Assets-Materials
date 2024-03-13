///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Raster.hpp"
#include "Scene.hpp"
#include "Camera.hpp"
#include <Langulus/Mesh.hpp>

using namespace Nodes;


/// Rasterizer node creation                                                  
///   @param desc - the rasterizer descriptor                                 
Raster::Raster(Describe&& descriptor)
   : Node {MetaOf<Raster>(), *descriptor} {
   // Extract settings                                                  
   mDescriptor.ExtractTrait<Traits::Bilateral>(mBilateral);
   mDescriptor.ExtractTrait<Traits::Signed>(mSigned);
   mDescriptor.ExtractTrait<Traits::Topology>(mTopology);
   mDescriptor.ExtractTrait<Traits::Min>(mDepth.mMin);
   mDescriptor.ExtractTrait<Traits::Max>(mDepth.mMax);

   // Extract rasterizer body                                           
   //mDescriptor.ExtractData(mCode);
   //LANGULUS_ASSERT(!mCode.IsEmpty(), Material, "No rasterizer code");

   if (not mTopology) {
      // Rasterizing triangles by default                               
      mTopology = MetaOf<A::Triangle>();
   }
}

/// Generate rasterizer definition code                                       
/// This is a 'fake' per-pixel rasterizer - very suboptimal, but useful in    
/// various scenarios                                                         
const Symbol& Raster::GeneratePerPixel() {
   // Generate children first                                           
   Descend();

   // In order to rasterize per pixel, we require child scene nodes     
   Symbols scenes;
   ForEachChild([&scenes](Nodes::Scene& scene) {
      scenes << scene.GenerateTriangles();
   });

   LANGULUS_ASSERT(scenes, Material, "No scenes available for rasterizer");

   if (scenes.GetCount() > 1)
      TODO(); // multiple triangle lists

   // Do face culling if required                                       
   GLSL culling;
   if (mBilateral)
      culling += "result.mFront = a <= 0.0;\n";
   else if (mSigned)
      culling += "if (a >= 0.0) return;";
   else
      culling += "if (a <= 0.0) return;";

   // Add rasterizer functions and dependencies                         
   AddDefine("RasterizeResult",
      RasterResult);
   AddDefine("RasterizeTriangle",
      Text::TemplateRt(RasterTriangle, culling));
   AddDefine("RasterizeTriangleList",
      Text::TemplateRt(RasterTriangleList, scenes[0].mCount, scenes[0].mCode));

   return ExposeData<Raster>("Rasterize({})", MetaOf<Camera>());
}

/// Generate fixed-pipeline rasterizer                                        
/// Uses the scene provided from the geometry shader, and rasterizes it using 
/// the default hardware rasterizer. Exposes symbols PerPixel as a result     
///   @return NoSymbol, as functionality is in the fixed-pipeline             
const Symbol& Raster::GeneratePerVertex() {
   auto position = GetSymbol<Traits::Place>(PerVertex);
   (void)position;

   AddDefine("gl_PerVertex",
      R"shader(
         out gl_PerVertex {
            vec4 gl_Position;
            float gl_PointSize;
            float gl_ClipDistance[];
         };
      )shader"
   );

   // We expose the built-in pixel shader inputs                        
   ExposeTrait<Traits::Place, Vec4>("gl_FragCoord");
   ExposeTrait<Traits::Place, Vec2>("gl_PointCoord");
   ExposeTrait<Traits::Signed, bool>("gl_FrontFacing");
   return NoSymbol;
}

/// Generate the rasterizer code                                              
///   @return the rasterizer function template                                
const Symbol& Raster::Generate() {
   Descend();

   if (mRate == PerVertex or mRate == PerPrimitive)
      return GeneratePerVertex();
   else if (mRate == PerPixel)
      return GeneratePerPixel();
   else
      LANGULUS_THROW(Material, "Bad rate for rasterizer");
}
