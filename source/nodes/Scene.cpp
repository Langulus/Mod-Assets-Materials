///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
///                                                                           
#include "Scene.hpp"
#include "../Material.hpp"
#include <Langulus/Mesh.hpp>
#include <Langulus/Math/Color.hpp>
#include <Langulus/Math/Normal.hpp>
#include <Langulus/Math/Sampler.hpp>

using namespace Nodes;


/// Scene node descriptor-constructor                                         
///   @param desc - the node descriptor                                       
Scene::Scene(Describe&& descriptor)
   : Resolvable {this}
   , Node {*descriptor} {
   // Notice how we don't satisfy the rest of the descriptor            
   // How the scene is generated depends on whether we're rasterizing,  
   // raymarching, raytracing, etc.                                     
   //InnerCreate();
}

/// Generate scene code                                                       
///   @return the SDF scene function template symbol                          
const Symbol& Scene::Generate() {
   Descend();

   // Intentionally does nothing. It is generated on demand by a        
   // raycaster/raymarcher/raytracer/rasterizer                         
   return NoSymbol;
}

/// Generate scene code                                                       
///   @return the array of lines symbol                                       
const Symbol& Scene::GenerateLines() {
   GLSL lines;
   Count countCombined = 0;

   // Get the lines of each geometry construct                          
   mDescriptor.ForEachConstruct([&](const Construct& c) {
      if (not c.CastsTo<A::Mesh>())
         return;

      // By default, geometry doesn't generate vertex positions         
      // and rasterizer requires it, so we create them                  
      // We generate color, too                                         
      auto geometryDescriptor = c;
      geometryDescriptor <<= MetaOf<A::Line>();
      geometryDescriptor <<= MetaOf<Vec3>();
      geometryDescriptor <<= MetaOf<RGBA>();

      // Get the generated geometry asset                               
      Verbs::Create creator {geometryDescriptor};
      const auto geometry = mMaterial->RunIn(creator)->As<A::Mesh*>();
      const auto count = geometry->GetLineCount();
      for (Count i = 0; i < count; ++i) {
         // Extract each line, and convert it to shader code            
         auto position = geometry->GetLineTrait<Traits::Place>(i);
         LANGULUS_ASSERT(position, Material,
            "Can't rasterize a line without Traits::Place");

         if (not position.template CastsTo<Vec3>(1))
            TODO();

         auto color = geometry->GetLineTrait<Traits::Color>(i);
         LANGULUS_ASSERT(color, Material,
            "Can't rasterize a line without Traits::Color");

         if (not color.template CastsTo<Vec4>(1))
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
   });

   LANGULUS_ASSERT(countCombined, Material, "No lines available");

   // Aggregate all lines in an array:                                  
   // const Line cLines[N] = Line[N](                                   
   //      Triangle(a, aColor, b, bColor),                              
   //      ... N times                                                  
   //   );                                                              
   AddDefine("Line", LineStruct);
   AddDefine("cLines", Text::TemplateRt(LineList, countCombined, lines));
   return ExposeData<Scene>("cLines");
}

/// Interpret a construct as an SDF function                                  
///   @param what - the construct to reinterpret                              
///   @param global - place where global definitions go                       
///   @return the generated scene function call                               
GLSL InterpretAsSDF(const Construct&, Material&) {
   TODO();
   return {};
}

/// Generate scene code                                                       
///   @return the SDF scene function template symbol                          
const Symbol& Scene::GenerateSDF() {
   GLSL scene;

   // Get the SDF code for each geometry construct                      
   mDescriptor.ForEachConstruct([&](const Construct& c) {
      if (not c.CastsTo<A::Mesh>())
         return;

      auto element = InterpretAsSDF(c, *mMaterial);
      if (not scene) {
         // This was the first element                                  
         scene = element;
         return;
      }

      // Each consecutive element is SDFUnion'ed                        
      // Define the union operation if not yet defined                  
      AddDefine("SDFUnion", SDFUnion);

      // Nest the union function for each new element                   
      scene = Text::TemplateRt(SDFUnionUsage, scene, element);
   });

   LANGULUS_ASSERT(scene, Material, "SDF scene is empty");

   // Define the scene function                                         
   AddDefine("Scene", Text::TemplateRt(SceneFunction, scene));

   // Expose scene usage                                                
   return ExposeTrait<Traits::D, float>("Scene({})", Traits::Place::OfType<Vec3>());
}

/// Generate scene code                                                       
///   @return the array of triangles symbol                                   
const Symbol& Scene::GenerateTriangles() {
   GLSL triangles;
   Count countCombined = 0;

   // Get the triangles of each geometry construct                      
   mDescriptor.ForEachConstruct([&](const Construct& c) {
      if (not c.CastsTo<A::Mesh>())
         return;

      // By default, geometry doesn't generate vertex positions         
      // and rasterizer requires it, so we create them                  
      // We generate color, too                                         
      auto geometryDescriptor = c;
      geometryDescriptor <<= MetaOf<A::Triangle>();
      geometryDescriptor <<= MetaOf<Vec3>();
      geometryDescriptor <<= MetaOf<Normal>();
      geometryDescriptor <<= MetaOf<Sampler2>();

      // Get the generated geometry asset                               
      Verbs::Create creator {geometryDescriptor};
      const auto geometry = mMaterial->RunIn(creator)->As<A::Mesh*>();
      const auto count = geometry->GetTriangleCount();
      for (Count i = 0; i < count; ++i) {
         auto position = geometry->template GetTriangleTrait<Traits::Place>(i);
         LANGULUS_ASSERT(position, Material,
            "Can't rasterize a triangle without Traits::Place");

         if (not position.template CastsTo<Vec3>(1))
            TODO();

         auto normal = geometry->template GetTriangleTrait<Traits::Aim>(i);
         LANGULUS_ASSERT(normal, Material,
            "Can't rasterize a triangle without Traits::Aim");

         if (not normal.template CastsTo<Vec3>(1))
            TODO();

         auto texture = geometry->GetTriangleTrait<Traits::Sampler>(i);
         LANGULUS_ASSERT(texture, Material,
            "Can't rasterize a triangle without Traits::Sampler");

         if (not texture.template CastsTo<Vec2>(1))
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
   });

   LANGULUS_ASSERT(countCombined, Material, "No triangles available");

   // Aggregate all triangles in an array:                              
   // const Triangle cTriangles[N] = Triangle[N](                       
   //      Triangle(a, aUV, b, bUV, c, cUV, n),                         
   //      ... N times                                                  
   //   );                                                              
   AddDefine("Triangle", TriangleStruct);
   AddDefine("cTriangles", Text::TemplateRt(TriangleList, countCombined, triangles));
   return ExposeData<Scene>("cTriangles");
}
