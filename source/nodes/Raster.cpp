///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "../MContent.hpp"

/// Rasterizer node creation																	
///	@param parent - the parent node														
///	@param verb - rasterizer creation verb												
MaterialNodeRasterize::MaterialNodeRasterize(MaterialNode* parent, const Verb& verb)
   : MaterialNode{ MetaData::Of<MaterialNodeRasterize>(), parent, verb } {
   if (mRate == RRate::PerVertex) {
      // We'll be using the fixed pipeline rasterizer							
      // Nothing to really do here													
      if (!verb.GetArgument().IsEmpty()) {
         pcLogSelfWarning 
            << "Using fixed-pipeline (PerVertex) rasterizer, "
            << "so the following argument(s) were ignored: "
            << verb.GetArgument();
      }
      return;
   }

   // Verb argument can provide additional traits for the rasterizer		
   verb.GetArgument().ForEachDeep([&](const Block& group) {
      EitherDoThis
      group.ForEach([&](const Trait& trait) {
         if (trait.TraitIs<Traits::Bilateral>()) {
            // Whether or not we're rasterizing double-sided triangles	
            PC_VERBOSE_MATERIAL("Configuring rasterizer: " << trait);
            mBilateral = trait.AsCast<bool>();
         }
         else if (trait.TraitIs<Traits::Signed>()) {
            // Whether or not we're reverting face winding					
            PC_VERBOSE_MATERIAL("Configuring rasterizer: " << trait);
            mSigned = trait.AsCast<bool>();
         }
         else if (trait.TraitIs<Traits::Topology>()) {
            // Configures the rasterizer to either triangles, or lines	
            PC_VERBOSE_MATERIAL("Configuring rasterizer: " << trait);
            mTopology = trait.AsCast<DataID>().GetMeta();
         }
         else if (trait.TraitIs<Traits::Min>()) {
            // Configures the rasterizer's min depth							
            PC_VERBOSE_MATERIAL("Configuring rasterizer: " << trait);
            mDepth.mMin = trait.AsCast<real>();
         }
         else if (trait.TraitIs<Traits::Max>()) {
            // Configures the rasterizer's max depth							
            PC_VERBOSE_MATERIAL("Configuring rasterizer: " << trait);
            mDepth.mMax = trait.AsCast<real>();
         }
      })
      OrThis
      group.ForEach([&](const GASM& code) {
         mCode = code;
      });
   });

   if (!mTopology)
      mTopology = ATriangle::Reflect();

   // Register the outputs																
   Expose<Traits::Color, real>("rasResult.mDepth");
   Expose<Traits::Sampler, vec2>("rasResult.mUV");
   Expose<Traits::Aim, vec3>("rasResult.mNormal");
   Expose<Traits::Texture, int>("int(rasResult.mFront)");
}

/// Generate rasterizer definition code													
/// This is a 'fake' per-pixel rasterizer - very suboptimal, but useful in		
/// various scenarios																			
void MaterialNodeRasterize::GeneratePerPixel() {
   // In order to rasterize, we require either triangle or line data		
   // Scan all parent nodes for scenes in order to generate these			
   GLSL triangleDef, sceneTriangles, sceneLines;
   pcptr triangleCount = 0, lineCount = 0;
   ForEachChild([&](MaterialNodeScene& scene) {
      scene.GenerateTriangleCode(triangleDef, sceneTriangles, triangleCount);
   });

   if (!lineCount && !triangleCount)
      throw Except::Content(pcLogSelfError << "No geometry available for rasterizer");

   GLSL define =
      "struct RasterizeResult {\n"
      "   vec3 mNormal;\n"
      "   vec2 mUV;\n"
      "   float mDepth;\n"_glsl +
      (mBilateral ? "   bool mFront;\n" : "") +
      "};\n\n";

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
///	@param vs - the vertex shader code													
template<RTTI::ReflectedTrait T>
void AddPerVertexOutput(GLSL& vs, const GLSL& symbol) {
   if (vs.Find("out gl_PerVertex {"))
      return;

   vs.Select(ShaderToken::Output)
      >> "out gl_PerVertex {\n";

   if constexpr (Same<T, Traits::Position>) {
      vs.Select(ShaderToken::Output)
         >> "\tvec4 gl_Position;\n";
      vs.Select(ShaderToken::Transform)
         >> "gl_Position = " + symbol + ";\n";
   }

   vs.Select(ShaderToken::Output)
      >> "};\n";

   //TODO 
   //float gl_PointSize;
   //float gl_ClipDistance[];
}

/// Generate fixed-pipeline rasterization													
/// Uses relevant attributes and rasterizes them, by forwarding them to the	
/// pixel shader																					
void MaterialNodeRasterize::GeneratePerVertex() {
   // If a pixel shader is missing, add a default one							
   GLSL& ps = GetProducer()->GetStage(ShaderStage::Pixel);
   if (ps.IsEmpty())
      ps = GLSL::From(ShaderStage::Pixel);

   // Wrap the output token															
   GLSL& vs = GetProducer()->GetStage(ShaderStage::Vertex);
   if (vs.IsEmpty())
      vs = GLSL::From(ShaderStage::Vertex);

   auto position = GetSymbol<Traits::Position>(RRate::PerVertex);
   if (!position.IsEmpty())
      AddPerVertexOutput<Traits::Position>(vs, position);

   //TODO 
   //float gl_PointSize;
   //float gl_ClipDistance[];
}

/// Generate the shader stages																
void MaterialNodeRasterize::Generate() {
   PC_VERBOSE_MATERIAL("Generating code...");

   if (!mCode.IsEmpty()) {
      auto parsed = mCode.Parse();
      Any context { GetBlock() };
      if (!Verb::ExecuteScope(context, parsed)) {
         throw Except::Content(pcLogSelfError
            << "Can't execute code: " << mCode);
      }
   }

   Descend();
   Consume();

   if (mRate == RRate::PerVertex)
      GeneratePerVertex();
   else if (mRate == RRate::PerPixel)
      GeneratePerPixel();
   else throw Except::Content(pcLogSelfError
      << "Bad rate for rasterizer");
}
