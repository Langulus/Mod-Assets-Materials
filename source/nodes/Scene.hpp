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
   struct Scene final : Node {
      LANGULUS(ABSTRACT) false;
      LANGULUS_BASES(Node);

      //Scene(Node*, const Descriptor&);
      Scene(const Neat&);

      const Symbol& Generate();
      const Symbol& GenerateSDF();
      const Symbol& GenerateLines();
      const Symbol& GenerateTriangles();
   };

} // namespace Nodes


///                                                                           
/// Lines                                                                     
///                                                                           

/// Line structure with color                                                 
constexpr Token LineStruct = R"shader(
   struct Line {
      vec3 a; vec4 aColor;
      vec3 b; vec4 bColor;
   };
)shader";

/// Line array                                                                
///   @param {0} number of lines                                              
///   @param {1] list of lines                                                
constexpr Token LineList = R"shader(
   const Line cLines[{0}] = Line[{0}]({1});
)shader";


/// SDF scene function                                                        
///   @param {0} - scene code                                                 
constexpr Token SceneFunction = R"shader(
   float Scene(in vec3 point) {{
      return {0};
   }}
)shader";

/// Signed distance union function                                            
constexpr Token SDFUnion = R"shader(
   float SDFUnion(float d1, float d2) {
      return min(d1,d2);
   }
)shader";

/// Signed distance union function usage                                      
///   @param {0} - first scene element                                        
///   @param {1} - second scene element                                       
constexpr Token SDFUnionUsage = R"shader(
   SDFUnion(
      {0},
      {1}
   )
)shader";


///                                                                           
/// Signed distance functions                                                 
///                                                                           

/// 3D box signed distance function                                           
constexpr Token SDFBox3 = R"shader(
   float SDF(in vec3 point, in Box3 box) {
      const vec3 d = abs(point) - box.mOffsets;
      return length(max(d, 0.0)) + min(max(d.x, max(d.y, d.z)), 0.0);
   };
)shader";

/// 2D box signed distance function                                           
constexpr Token SDFBox2 = R"shader(
   float SDF(in vec2 point, in Box2 box) {
      const vec2 d = abs(point) - box.mOffsets;
      return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
   };
)shader";

/// 3D rounded box signed distance function                                   
constexpr Token SDFBoxRounded3 = R"shader(
   float SDF(in vec3 point, in BoxRounded3 box) {
      const vec3 d = abs(point) - box.mOffsets;
      return length(max(d, 0.0)) + min(max(d.x, max(d.y, d.z)), 0.0) - box.mRadius;
   };
)shader";

/// 2D rounded box signed distance function                                   
constexpr Token SDFBoxRounded2 = R"shader(
   float SDF(in vec2 point, in BoxRounded2 box) {
      const vec2 d = abs(point) - box.mOffsets;
      return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0) - box.mRadius;
   };
)shader";

/// Cone signed distance function (extends in X direction)                    
constexpr Token SDFConeX = R"shader(
   float SDF(in vec3 point, in ConeX cone) {
      const float q = length(point.yz);
      const vec2 c {sin(cone.mAngle), cos(cone.mAngle)};
      return max(dot(c, vec2(q, point.x)), -cone.mHeight - point.x);
   };
)shader";

/// Cone signed distance function (extends in Y direction)                    
constexpr Token SDFConeY = R"shader(
   float SDF(in vec3 point, in ConeY cone) {
      const float q = length(point.xz);
      const vec2 c {sin(cone.mAngle), cos(cone.mAngle)};
      return max(dot(c, vec2(q, point.y)), -cone.mHeight - point.y);
   };
)shader";

/// Cone signed distance function (extends in Z direction)                    
constexpr Token SDFConeZ = R"shader(
   float SDF(in vec3 point, in ConeZ cone) {
      const float q = length(point.xy);
      const vec2 c {sin(cone.mAngle), cos(cone.mAngle)};
      return max(dot(c, vec2(q, point.z)), -cone.mHeight - point.z);
   };
)shader";

/// Cylinder signed distance function (extends in X direction)                
constexpr Token SDFCylinderX = R"shader(
   float SDF(in vec3 point, in CylinderX cyl) {
      return length(point.yz) - cyl.mRadius;
   };
)shader";

/// Cylinder signed distance function (extends in Y direction)                
constexpr Token SDFCylinderY = R"shader(
   float SDF(in vec3 point, in CylinderY cyl) {
      return length(point.xz) - cyl.mRadius;
   };
)shader";

/// Cylinder signed distance function (extends in Z direction)                
constexpr Token SDFCylinderZ = R"shader(
   float SDF(in vec3 point, in CylinderZ cyl) {
      return length(point.xy) - cyl.mRadius;
   };
)shader";


///                                                                           
/// Triangles                                                                 
///                                                                           

/// Triangle structure with texture coordinates and a normal                  
constexpr Token TriangleStruct = R"shader(
   struct Triangle {
      vec3 a; vec2 aUV;
      vec3 b; vec2 bUV;
      vec3 c; vec2 cUV;
      vec3 n;
   };
)shader";

/// Triangle array                                                            
///   @param {0} number of triangles                                          
///   @param {1] list of triangles                                            
constexpr Token TriangleList = R"shader(
   const Triangle cTriangles[{0}] = Triangle[{0}]({1});
)shader";
