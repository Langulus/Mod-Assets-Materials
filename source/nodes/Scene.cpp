///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Scene.hpp"

using namespace Nodes;


/// Scene node as member constructor                                          
///   @param parent - the owning node                                         
///   @param desc - the node descriptor                                       
Scene::Scene(Node* parent, const Descriptor& desc)
   : Node {MetaOf<Scene>(), parent, desc} {}

/// Scene node descriptor-constructor                                         
///   @param desc - the node descriptor                                       
Scene::Scene(const Descriptor& desc)
   : Node {MetaOf<Scene>(), desc} {}

/// For logging                                                               
Scene::operator Debug() const {
   Code result;
   result += Node::DebugBegin();
      //result += pcSerialize<Debug>(mGeometry);
   result += Node::DebugEnd();
   return result;
}

/// Generate scene code                                                       
///   @return the SDF scene function template symbol                          
Symbol& Scene::Generate() {
   Descend();

   // Intentionally does nothing. It is generated on demand by a        
   // raycaster/raymarcher/raytracer/rasterizer                         
   TODO();
}


/// Generate scene code                                                       
///   @return the array of lines symbol                                       
Symbol& Scene::GenerateLines() {
   GLSL lines;
   Count countCombined;

   // Get the lines of each geometry construct                          
   for (auto pair : mDescriptor.mConstructs) {
      if (!pair.mKey->template CastsTo<A::Geometry>())
         continue;

      for (auto& construct : pair.mValue) {
         // By default, geometry doesn't generate vertex positions      
         // and rasterizer requires it, so we create them               
         // We generate color, too                                      
         Any additional {
            MetaOf<A::Line>(),
            MetaOf<Point3>(),
            MetaOf<RGBA>()
         };

         Any unusedSideproducts;
         Any geometryBlock {mGeometry->GetBlock()};
         Verb::DefaultCreateInner(geometryBlock, additional, unusedSideproducts);
         mGeometry->Generate();

         const auto count = mGeometry->GetLineCount();
         for (Count i = 0; i < count; ++i) {
            auto position = mGeometry->GetLineTrait<Traits::Place>(i);
            LANGULUS_ASSERT(!position.IsEmpty(), Material,
               "Can't rasterize a line without Traits::Place");

            if (!position.template CastsTo<Vec3>(1))
               TODO();

            auto color = mGeometry->GetLineTrait<Traits::Color>(i);
            LANGULUS_ASSERT(!color.IsEmpty(), Material,
               "Can't rasterize a line without Traits::Color");

            if (!color.template CastsTo<Vec4>(1))
               TODO();

            if (countCombined > 0)
               lines += ", \n";
            lines += "     Line(";
            lines += position.As<Vec3>(0);
            lines += ", ";
            lines += color.As<Vec4>(0);
            lines += ", ";
            lines += position.As<Vec3>(1);
            lines += ", ";
            lines += color.As<Vec4>(1);
            lines += ")";
            ++countCombined;
         }
      }
   }

   LANGULUS_ASSERT(countCombined, Material, "No lines available");

   // Aggregate all lines in an array:                                  
   // const Line cLines[N] = Line[N](                                   
   //      Triangle(a, aColor, b, bColor),                              
   //      ... N times                                                  
   //   );                                                              
   AddDefine("Line", LineStruct);
   AddDefine("cLines", TemplateFill(LineList, countCombined, lines));
   return Expose<Scene>("cLines");
}

/// Interpret a construct as an SDF function                                  
///   @param what - the construct to reinterpret                              
///   @param global - place where global definitions go                       
///   @return the generated scene function call                               
GLSL InterpretAsSDF(const Construct& what, Material& global) {
   TODO();
}

/// Generate scene code                                                       
///   @return the SDF scene function template symbol                          
Symbol& Scene::GenerateSDF() {
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

   // Expose scene usage                                                
   return Expose<Traits::D>("Scene({})", Traits::Place::OfType<Vec3>());
}

/// Generate scene code                                                       
///   @return the array of triangles symbol                                   
Symbol& Scene::GenerateTriangles() {
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
         Any additional {
            MetaOf<A::Triangle>(),
            MetaOf<Point3>(),
            MetaOf<Normal>(),
            MetaOf<Sampler2>()
         };

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

            if (countCombined > 0)
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
   return Expose<Scene>("cTriangles");
}
