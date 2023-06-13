///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Scene.hpp"

using namespace Nodes;


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


/// Generate scene code                                                       
///   @return the array of triangles symbol                                   
Symbol Scene::GenerateTriangles() {
   GLSL triangles;
   Count countCombined;

   // Get the triangles of each geometry construct                      
   for (auto pair : mDescriptor.mConstructs) {
      if (!pair.mKey->template CastsTo<A::Geometry>())
         continue;

      for (auto& construct : pair.mValue) {
         // By default, geometry doesn't generate vertex positions      
         // and rasterizer requires it, so we create them               
         // We generate normals and texture coordinates, too            
         auto additional = Any::Wrap(
            MetaOf<A::Triangle>,
            MetaOf<Point3>,
            MetaOf<Normal>,
            MetaOf<Sampler2>
         );

         Any unusedSideproducts;
         Any geometryBlock {mGeometry->GetBlock()};
         Verb::DefaultCreateInner(geometryBlock, additional, unusedSideproducts);
         mGeometry->Generate();

         const auto count = mGeometry->GetTriangleCount();
         for (Count i = 0; i < count; ++i) {
            auto position = mGeometry->GetTriangleTrait<Traits::Place>(i);
            LANGULUS_ASSERT(!position.IsEmpty(), Material,
               "Can't rasterize a triangle without Traits::Place");

            if (!position.template CastsTo<Vec3>(1))
               TODO();

            auto normal = mGeometry->GetTriangleTrait<Traits::Aim>(i);
            LANGULUS_ASSERT(!normal.IsEmpty(), Material,
               "Can't rasterize a triangle without Traits::Aim");

            if (!normal.template CastsTo<Vec3>(1))
               TODO();

            auto texture = mGeometry->GetTriangleTrait<Traits::Sampler>(i);
            LANGULUS_ASSERT(!texture.IsEmpty(), Material,
               "Can't rasterize a triangle without Traits::Sampler");

            if (!texture.template CastsTo<Vec2>(1))
               TODO();

            if (triangleCount > 0)
               triangles += ", \n";
            triangles += "     Triangle(";
            triangles += position.As<Vec3>(0);
            triangles += ", ";
            triangles += texture.As<Vec2>(0);
            triangles += ", ";
            triangles += position.As<Vec3>(1);
            triangles += ", ";
            triangles += texture.As<Vec2>(1);
            triangles += ", ";
            triangles += position.As<Vec3>(2);
            triangles += ", ";
            triangles += texture.As<Vec2>(2);
            triangles += ", ";
            triangles += normal.As<Vec3>(0);
            triangles += ")";
            ++countCombined;
         }
      }
   }

   LANGULUS_ASSERT(countCombined, Material, "No triangles available");

   // Aggregate all triangles in an array:                              
   // const Triangle cTriangles[N] = Triangle[N](                       
   //      Triangle(a, aUV, b, bUV, c, cUV, n),                         
   //      ... N times                                                  
   //   );                                                              
   AddDefine("Triangle", TriangleStruct);
   AddDefine("cTriangles", TemplateFill(TriangleList, countCombined, triangles));
}
