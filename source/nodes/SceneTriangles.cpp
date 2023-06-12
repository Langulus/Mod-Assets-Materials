///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Scene.hpp"

using namespace Nodes;


/// Triangle structure                                                        
constexpr Token TriangleStruct = R"shader(
   struct Triangle {
      vec3 a; vec2 aUV;
      vec3 b; vec2 bUV;
      vec3 c; vec2 cUV;
      vec3 n;
   };
)shader";


/// Generate scene code                                                       
///   @return the array of triangles symbol                                   
Symbol Scene::GenerateTriangles() {
   // Get the triangle code for each geometry construct                 
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

         // Triangles                                                   
         mMaterial->AddDefine("Triangle", TriangleStruct);

         // Aggregate all triangles in an array of sorts:               
         // const Triangle cTriangles[N] = Triangle[N](                 
         //      Triangle(a, aUV, b, bUV, c, cUV, n),                   
         //      ... N times                                            
         //   );                                                        
         const auto count = mGeometry->GetTriangleCount();
         for (Count i = 0; i < count; ++i) {
            auto position = mGeometry->GetTriangleTrait<Traits::Place>(i);
            if (position.IsEmpty())
               LANGULUS_THROW(Material, "Can't rasterize a triangle without Traits::Place");

            if (!position.template CastsTo<Vec3>(1))
               TODO();

            auto normal = mGeometry->GetTriangleTrait<Traits::Aim>(i);
            if (normal.IsEmpty())
               LANGULUS_THROW(Material, "Can't rasterize a triangle without Traits::Aim");

            if (!normal.template CastsTo<Vec3>(1))
               TODO();

            auto texture = mGeometry->GetTriangleTrait<Traits::Sampler>(i);
            if (texture.IsEmpty())
               LANGULUS_THROW(Material, "Can't rasterize a triangle without Traits::Sampler");

            if (!texture.template CastsTo<Vec2>(1))
               TODO();

            static const GLSL separator = ", ";
            if (triangleCount > 0) {
               sceneTriangles += separator;
               sceneTriangles += "\n";
            }

            sceneTriangles += "   Triangle(";
            sceneTriangles += position.As<Vec3>(0);
            sceneTriangles += ", ";
            sceneTriangles += texture.As<Vec2>(0);
            sceneTriangles += ", ";
            sceneTriangles += position.As<Vec3>(1);
            sceneTriangles += ", ";
            sceneTriangles += texture.As<Vec2>(1);
            sceneTriangles += ", ";
            sceneTriangles += position.As<Vec3>(2);
            sceneTriangles += ", ";
            sceneTriangles += texture.As<Vec2>(2);
            sceneTriangles += ", ";
            sceneTriangles += normal.As<Vec3>(0);
            sceneTriangles += ")";
            ++triangleCount;
         }
      }
   }
}
