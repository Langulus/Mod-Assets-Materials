///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "../Node.hpp"
#include <Math/Range.hpp>


namespace Nodes
{

   ///                                                                        
   ///   Rasterizer material node                                             
   ///                                                                        
   struct Raster final : Node {
      LANGULUS(ABSTRACT) false;
      LANGULUS_BASES(Node);

   private:
      // Code for the rasterizer                                        
      //Code mCode;
      // Whether or not to rasterize both sides of triangles            
      bool mBilateral {};
      // Whether or not triangle faces are flipped                      
      bool mSigned {};
      // Whether we're rasterizing triangles or lines                   
      DMeta mTopology {};
      // The depth range in which we're rasterizing                     
      Range1 mDepth {0, 1000};

   public:
      Raster(const Neat&);
      const Symbol& Generate();

   private:
      const Symbol& GeneratePerPixel();
      const Symbol& GeneratePerVertex();
   };

} // namespace Nodes


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