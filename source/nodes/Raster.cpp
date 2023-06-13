///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Raster.hpp"
#include "Scene.hpp"

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

/// Rasterize single triangle                                                 
///   @param {0} - culling and sidedness code                                 
constexpr Token RasterTriangle = R"shader(
   void RasterizeTriangle(in CameraResult camera, in Triangle triangle, inout RasterizeResult result) {{
      // Transform to eye space
      vec4 pt0 = camera.mProjectedView * Transform(triangle.a);
      vec4 pt1 = camera.mProjectedView * Transform(triangle.b);
      vec4 pt2 = camera.mProjectedView * Transform(triangle.c);

      vec2 p0 = pt0.xy / pt0.w;
      vec2 p1 = pt1.xy / pt1.w;
      vec2 p2 = pt2.xy / pt2.w;

      float a = 0.5 * (-p1.y * p2.x + p0.y * (-p1.x + p2.x) + p0.x * (p1.y - p2.y) + p1.x * p2.y);

      {0}

      float s = 1.0 / (2.0 * a) * (p0.y * p2.x - p0.x * p2.y
         + (p2.y - p0.y) *  camera.mScreenUV.x
         + (p0.x - p2.x) * -camera.mScreenUV.y
      );
      float t = 1.0 / (2.0 * a) * (p0.x * p1.y - p0.y * p1.x
         + (p0.y - p1.y) *  camera.mScreenUV.x
         + (p1.x - p0.x) * -camera.mScreenUV.y
      );

      if (s <= 0.0 || t <= 0.0 || 1.0 - s - t <= 0.0)
         return;

      vec4 test = vec4(1.0, s, t, 1.0 - s - t);
      float denominator = 1.0 / (test.y / pt1.w + test.z / pt2.w + test.w / pt0.w);
      float z = (
         (pt1.z * test.y) / pt1.w +
         (pt2.z * test.z) / pt2.w +
         (pt0.z * test.w) / pt0.w
      ) * denominator;

      if (z < result.mDepth && z > 0.0) {{
         result.mUV.x = (
            (triangle.bUV.x * test.y) / pt1.w +
            (triangle.cUV.x * test.z) / pt2.w +
            (triangle.aUV.x * test.w) / pt0.w
         ) * denominator;

         result.mUV.y = (
            (triangle.bUV.y * test.y) / pt1.w +
            (triangle.cUV.y * test.z) / pt2.w +
            (triangle.aUV.y * test.w) / pt0.w
         ) * denominator;

         result.mDepth = z;
         result.mNormal = triangle.n;
      }}
   }}
)shader";


/// Rasterize single line                                                     
///   @param {0} - culling and sidedness code                                 
constexpr Token RasterLine = R"shader(
   void RasterizeLine(in CameraResult camera, in Line line, inout RasterizeResult result) {{
      TODO
   }}
)shader";


/// Rasterize a list of triangles                                             
///   @param {0} - number of triangles                                        
///   @param {1} - triangle array name                                        
constexpr Token RasterTriangleList = R"shader(
   void RasterizeTriangleList(in CameraResult camera, inout RasterizeResult result) {
      for (int i = 0; i < {0}; i += 1) {{
         RasterizeTriangle(camera, {1}[i], result);
      }}
   }}
)shader";

/// Rasterize a list of lines                                                 
///   @param {0} - number of lines                                            
///   @param {1} - line array name                                            
constexpr Token RasterLineList = R"shader(
   void RasterizeLineList(in CameraResult camera, inout RasterizeResult result) {
      for (int i = 0; i < {0}; i += 1) {{
         RasterizeLine(camera, {1}[i], result);
      }}
   }}
)shader";

/// Rasterizer usage snippet                                                  
///   @param {0} - max depth                                                  
constexpr Token RasterUsage = R"shader(
   RasterizeResult rasResult;
   rasResult.mDepth = {0};
   SceneRAS(camResult, rasResult);
   if (rasResult.mDepth >= {0})
      discard;
   rasResult.mDepth = 1.0 - rasResult.mDepth / {0};
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

   // Do face culling if required                                       
   GLSL culling;
   if (mBilateral)
      culling += "result.mFront = a <= 0.0;\n";
   else if (mSigned)
      culling += "if (a >= 0.0) return;";
   else
      culling += "if (a <= 0.0) return;";

   // Define the triangle rasterizing function                          
   GLSL rasterizeTriangle = TemplateFill(RasterTriangles, culling);

   // Finish up the SceneRAS() if anything was defined in it            
   GLSL sceneFunction = "void SceneRAS(in CameraResult camera, inout RasterizeResult result) {\n";
   if (!sceneTriangles.IsEmpty()) {
      // Finish up the triangle array                                   
      sceneTriangles = triangleDef 
         + "const Triangle cTriangles[" + triangleCount + "] = Triangle["
         + triangleCount + "](\n" + sceneTriangles + "\n);\n\n";

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

      // Add the required rasterizer to the definitions
      define += sceneLines;
      if (!define.Find(rasterizeLine))
         define += rasterizeLine;
   }

   define += sceneFunction;

   // Rasterize usage                                                   
   GLSL usage = TemplateFill(RasterLineList, mDepth.mMax);
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
Symbol Raster::Generate() {
   Descend();

   if (mRate == PerVertex)
      GeneratePerVertex();
   else if (mRate == PerPixel)
      GeneratePerPixel();
   else
      LANGULUS_THROW(Material, "Bad rate for rasterizer");

   return Expose<Raster>("Rasterize({})", MetaOf<Camera>());
}
