///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Raster.hpp"
#include "SceneTriangles.hpp"
#include "SceneLines.hpp"

using namespace Nodes;


/// Rasterizer result                                                         
constexpr Token RasterResult = R"shader(
   struct RasterizeResult {
      vec3 mNormal;
      vec2 mUV;
      float mDepth;
      int mTextureId;
   };
)shader";


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

   // Parse scene                                                       
   if (mTopology->CastsTo<A::Triangle>())
      mScene = new SceneTriangles {this, desc};
   else if (mTopology->CastsTo<A::Line>())
      mScene = new SceneLines {this, desc};
   else
      LANGULUS_THROW(Material, "Bad topology for rasterizer");

   // Register the outputs                                              
   Expose<Traits::Color, Real>("rasResult.mDepth");
   Expose<Traits::Sampler, Vec2>("rasResult.mUV");
   Expose<Traits::Aim, Vec3>("rasResult.mNormal");
   Expose<Traits::Texture, int>("int(rasResult.mFront)");
}

Raster::~Raster() {
   if (mScene)
      delete mScene;
}

/// Generate rasterizer definition code                                       
/// This is a 'fake' per-pixel rasterizer - very suboptimal, but useful in    
/// various scenarios                                                         
void Raster::GeneratePerPixel() {
   // In order to rasterize, we require a scene                         
   auto scene = mScene.Generate();
   LANGULUS_ASSERT(!scene.IsEmpty(), Material,
      "No scene available for rasterizer");

   // In order to rasterize, we require either triangle or line data    
   // Scan all parent nodes for scenes in order to generate these       
   GLSL triangleDef, sceneTriangles, sceneLines;
   Count triangleCount = 0, lineCount = 0;
   ForEachChild([&](MaterialNodeScene& scene) {
      scene.GenerateTriangleCode(triangleDef, sceneTriangles, triangleCount);
   });

   if (!lineCount && !triangleCount)
      throw Except::Content(pcLogSelfError << "No geometry available for rasterizer");

   GLSL define = RasterResult;

   // Define the triangle rasterizing function                           
   GLSL rasterizeTriangle =
      "void RasterizeTriangle(in CameraResult camera, in Triangle triangle, inout RasterizeResult result) {\n"
      "   // Transform to eye space\n"
      "   vec4 pt0 = camera.mProjectedView * Transform(triangle.a);\n"
      "   vec4 pt1 = camera.mProjectedView * Transform(triangle.b);\n"
      "   vec4 pt2 = camera.mProjectedView * Transform(triangle.c);\n\n"

      "   vec2 p0 = pt0.xy / pt0.w;\n"
      "   vec2 p1 = pt1.xy / pt1.w;\n"
      "   vec2 p2 = pt2.xy / pt2.w;\n\n"

      "   float a = 0.5 * (-p1.y * p2.x + p0.y * (-p1.x + p2.x) + p0.x * (p1.y - p2.y) + p1.x * p2.y);\n";

   // Do face culling if required                                       
   if (mBilateral)
      rasterizeTriangle += "   result.mFront = a <= 0.0;\n";
   else if (mSigned)
      rasterizeTriangle += "   if (a >= 0.0) return;\n";
   else
      rasterizeTriangle += "   if (a <= 0.0) return;\n";

   rasterizeTriangle +=
      "   float s = 1.0 / (2.0 * a) * (p0.y * p2.x - p0.x * p2.y \n"
      "      + (p2.y - p0.y) * camera.mScreenUV.x \n"
      "      + (p0.x - p2.x) * -camera.mScreenUV.y);\n"
      "   float t = 1.0 / (2.0 * a) * (p0.x * p1.y - p0.y * p1.x \n"
      "      + (p0.y - p1.y) * camera.mScreenUV.x \n"
      "      + (p1.x - p0.x) * -camera.mScreenUV.y);\n\n"

      "   if (s <= 0.0 || t <= 0.0 || 1.0 - s - t <= 0.0) \n"
      "      return;\n\n"

      "   vec4 test = vec4(1.0, s, t, 1.0 - s - t); \n"
      "   float denominator = 1.0 / (test.y / pt1.w + test.z / pt2.w + test.w / pt0.w);\n"
      "   float z = (\n"
      "      (pt1.z * test.y) / pt1.w + \n"
      "      (pt2.z * test.z) / pt2.w + \n"
      "      (pt0.z * test.w) / pt0.w \n"
      "   ) * denominator; \n\n"

      "   if (z < result.mDepth && z > 0.0) {\n"
      "      result.mUV.x = (\n"
      "         (triangle.bUV.x * test.y) / pt1.w + \n"
      "         (triangle.cUV.x * test.z) / pt2.w + \n"
      "         (triangle.aUV.x * test.w) / pt0.w \n"
      "      ) * denominator; \n\n"

      "      result.mUV.y = (\n"
      "         (triangle.bUV.y * test.y) / pt1.w + \n"
      "         (triangle.cUV.y * test.z) / pt2.w + \n"
      "         (triangle.aUV.y * test.w) / pt0.w \n"
      "      ) * denominator; \n\n"

      "      result.mDepth = z;\n"
      "      result.mNormal = triangle.n;\n"
      "   }\n"
      "}\n\n";

   // Define the line rasterizing function                              
   static const GLSL rasterizeLine =
      "void RasterizeLine(in CameraResult camera, in Line t, inout RasterizeResult result) {\n"
      "   TODO();\n"
      "}\n\n";

   // Finish up the SceneRAS() if anything was defined in it            
   GLSL sceneFunction = "void SceneRAS(in CameraResult camera, inout RasterizeResult result) {\n";
   if (!sceneTriangles.IsEmpty()) {
      // Finish up the triangle array                                   
      sceneTriangles = triangleDef 
         + "const Triangle cTriangles[" + triangleCount + "] = Triangle["
         + triangleCount + "](\n" + sceneTriangles + "\n);\n\n";

      // And iterate the triangles inside the SceneRAS()                
      sceneFunction += "   for(int triangleIndex = 0; triangleIndex < ";
      sceneFunction += triangleCount;
      sceneFunction += "; triangleIndex += 1) {\n";
      sceneFunction += "      RasterizeTriangle(camera, cTriangles[triangleIndex], result);\n";
      sceneFunction += "   }\n";

      // Add the required rasterizer to the definitions                 
      define += sceneTriangles;
      if (!define.Find(rasterizeTriangle))
         define += rasterizeTriangle;
   }

   if (!sceneLines.IsEmpty()) {
      // Finish up the line array
      /*sceneLines = lineDef 
         + "const Lines cLines[" + lineCount + "] = Line["
         + lineCount + "](\n" + sceneLines + "\n);\n\n";*/

      // And iterate the lines inside the SceneRAS()
      sceneFunction += "   for(int lineIndex = 0; lineIndex < ";
      sceneFunction += lineCount;
      sceneFunction += "; lineIndex += 1) {\n";
      sceneFunction += "      RasterizeLine(camera, cLines[lineIndex], result);\n";
      sceneFunction += "   }\n";

      // Add the required rasterizer to the definitions
      define += sceneLines;
      if (!define.Find(rasterizeLine))
         define += rasterizeLine;
   }

   sceneFunction += "}\n\n";
   define += sceneFunction;

   // Rasterize usage                                                   
   GLSL usage = 
      "RasterizeResult rasResult;\n"
      "rasResult.mDepth = "_glsl + mDepth.mMax + ";\n"
      "SceneRAS(camResult, rasResult);\n"
      "if (rasResult.mDepth >= " + mDepth.mMax + ")\n"
      "   discard;\n"
      "rasResult.mDepth = 1.0 - rasResult.mDepth / " + mDepth.mMax + ";\n\n";

   Commit(ShaderToken::Functions, define);
   Commit(ShaderToken::Transform, usage);
}

/// Wrap a per vertex symbol to the built-in gl_PerVertex structure           
///   @param vs - the vertex shader code                                      
template<CT::Trait T>
void AddPerVertexOutput(GLSL& vs, const GLSL& symbol) {
   if (vs.Find("out gl_PerVertex {"))
      return;

   vs.Select(ShaderToken::Output) >> "out gl_PerVertex {\n";

   if constexpr (CT::Same<T, Traits::Position>) {
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
Symbol Raster::Generate() {
   Descend();

   if (mRate == PerVertex)
      GeneratePerVertex();
   else if (mRate == PerPixel)
      GeneratePerPixel();
   else
      LANGULUS_THROW(Material, "Bad rate for rasterizer");
}
