///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "SceneSDF.hpp"

using namespace Nodes;


/// Signed distance union function                                            
constexpr Token SDFUnion = R"shader(
   float SDFUnion(float d1, float d2) {
      return min(d1,d2);
   }
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

GLSL InterpretAsSDF(const Construct& what, GLSL& dep) {
   TODO();
}

/// Scene node descriptor-constructor                                         
///   @param desc - the node descriptor                                       
SceneSDF::SceneSDF(const Descriptor& desc)
   : Node {MetaOf<SceneSDF>(), desc} {}

/// For logging                                                               
SceneSDF::operator Debug() const {
   Code result;
   result += Node::DebugBegin();
      //result += pcSerialize<Debug>(mGeometry);
   result += Node::DebugEnd();
   return result;
}

/// Generate code for signed distance field from geometry                     
///   @param define - [in/out] code definitions                               
///   @param scene - [out] SDF code goes here                                 
void SceneSDF::GenerateCode(GLSL& define, GLSL& scene) {
   // Scene nodes are usually inside Raster/Raycast/Raymarch/Raytrace   
   // nodes, and the place trait should come from there                 
   auto symPos = GetSymbol<Traits::Place, Vec3>(PerPixel);

   // Get the SDF code for each geometry construct                      
   for (auto pair : mDescriptor.mConstructs) {
      if (!pair.mKey->template CastsTo<A::Geometry>())
         continue;

      for (auto& construct : pair.mValue) {
         GLSL element = InterpretAsSDF(construct, define);
         if (scene.IsEmpty()) {
            // This was the first element                               
            scene += element;
         }
         else {
            // Define the union operation if not yet defined            
            if (!define.Find(SDFUnion))
               define += SDFUnion;

            // Nest the union function for each new element             
            scene = "SDFUnion(" + scene;
            scene += ", ";
            scene += element;
            scene += ')';
         }
      }
   }
}

/// Generate the shader stages                                                
void SceneSDF::Generate() {
   VERBOSE_NODE("Generating code...");
   Descend();
   Consume();
}
