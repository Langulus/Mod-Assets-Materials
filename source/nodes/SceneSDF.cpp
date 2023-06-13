///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Scene.hpp"

using namespace Nodes;


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

/// Interpret a construct as an SDF function                                  
///   @param what - the construct to reinterpret                              
///   @param global - place where global definitions go                       
///   @return the generated scene function call                               
GLSL InterpretAsSDF(const Construct& what, Material& global) {
   TODO();
}

/// Generate scene code                                                       
///   @return the SDF scene function template symbol                          
Symbol Scene::GenerateSDF() {
   GLSL scene;

   // Get the SDF code for each geometry construct                      
   for (auto pair : mDescriptor.mConstructs) {
      if (!pair.mKey->template CastsTo<A::Geometry>())
         continue;

      for (auto& construct : pair.mValue) {
         auto element = InterpretAsSDF(construct, *mMaterial);
         if (scene.IsEmpty()) {
            // This was the first element                               
            scene = element;
            continue;
         }

         // Each consecutive element is SDFUnion'ed                     
         // Define the union operation if not yet defined               
         AddDefine("SDFUnion", SDFUnion);

         // Nest the union function for each new element                
         scene = TemplateFill(SDFUnionUsage, scene, element);
      }
   }

   LANGULUS_ASSERT(!scene.IsEmpty(), Material, "SDF scene is empty");

   // Define the scene function                                         
   AddDefine("Scene", TemplateFill(SceneFunction, scene));
}
