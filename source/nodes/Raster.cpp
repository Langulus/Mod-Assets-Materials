//                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Raster.hpp"
#include "Scene.hpp"

using namespace Nodes;


/// Rasterizer node creation                                                  
///   @param desc - the rasterizer descriptor                                 
Raster::Raster(const Descriptor& desc)
   : Node {MetaOf<Raster>(), desc} {
   // Extract settings                                                  
   mDescriptor.ExtractTrait<Traits::Bilateral>(mBilateral);
   mDescriptor.ExtractTrait<Traits::Signed>(mSigned);
   mDescriptor.ExtractTrait<Traits::Topology>(mTopology);
   mDescriptor.ExtractTrait<Traits::Min>(mDepth.mMin);
   mDescriptor.ExtractTrait<Traits::Max>(mDepth.mMax);

   // Extract rasterizer body                                           
   mDescriptor.ExtractData(mCode);
   LANGULUS_ASSERT(!mCode.IsEmpty(), Material, "No rasterizer code");

   if (!mTopology) {
      // Rasterizing triangles by default                               
      mTopology = MetaOf<A::Triangle>();
   }
}

/// Generate rasterizer definition code                                       
/// This is a 'fake' per-pixel rasterizer - very suboptimal, but useful in    
/// various scenarios                                                         
void Raster::GeneratePerPixel() {
   // Generate children first                                           
   Descend();

   // In order to raymarch, we require child scene nodes                
   Symbols scenes;
   ForEachChild([&scenes](Nodes::Scene& scene) {
      scenes << scene.GenerateTriangles();
   });

   LANGULUS_ASSERT(!scenes.IsEmpty(), Material,
      "No scenes available for rasterizer");

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
   AddDefine("RasterizeResult", RasterResult);
   AddDefine("RasterizeTriangle", TemplateFill(RasterTriangles, culling));
   AddDefine("RasterizeTriangleList", TemplateFill(RasterizeTriangleList, scenes[0].mCount, scenes[0].mCode));

   return Expose<Raster>("Rasterize({})", MetaOf<Camera>());
}

/// Wrap a per vertex symbol to the built-in gl_PerVertex structure           
///   @param vs - the vertex shader code                                      
template<CT::Trait T>
void AddPerVertexOutput(GLSL& vs, const GLSL& symbol) {
   if (vs.Find("out gl_PerVertex {"))
      return;

   vs.Select(ShaderToken::Output) >> "out gl_PerVertex {\n";

   if constexpr (CT::Same<T, Traits::Place>) {
      vs.Select(ShaderToken::Output)      >> "\tvec4 gl_Position;\n";
      vs.Select(ShaderToken::Transform)   >> "gl_Position = " + symbol + ";\n";
   }

   vs.Select(ShaderToken::Output) >> "};\n";

   //TODO 
   //float gl_PointSize;
   //float gl_ClipDistance[];
}

/// Generate fixed-pipeline rasterization                                     
/// Uses relevant attributes and rasterizes them, by forwarding them to the   
/// pixel shader                                                              
void Raster::GeneratePerVertex() {
   // If a pixel shader is missing, add a default one                   
   GLSL& ps = GetProducer()->GetStage(ShaderStage::Pixel);
   if (ps.IsEmpty())
      ps = GLSL::From(ShaderStage::Pixel);

   // Wrap the output token                                             
   GLSL& vs = GetProducer()->GetStage(ShaderStage::Vertex);
   if (vs.IsEmpty())
      vs = GLSL::From(ShaderStage::Vertex);

   auto position = GetSymbol<Traits::Place>(PerVertex);
   if (!position.IsEmpty())
      AddPerVertexOutput<Traits::Place>(vs, position);

   //TODO 
   //float gl_PointSize;
   //float gl_ClipDistance[];
}

/// Generate the rasterizer code                                              
///   @return the rasterizer function template                                
Symbol& Raster::Generate() {
   Descend();

   if (mRate == PerVertex)
      GeneratePerVertex();
   else if (mRate == PerPixel)
      GeneratePerPixel();
   else
      LANGULUS_THROW(Material, "Bad rate for rasterizer");

   return Expose<Raster>("Rasterize({})", MetaOf<Camera>());
}
