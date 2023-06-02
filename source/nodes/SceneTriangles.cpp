///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "SceneTriangles.hpp"

using namespace Nodes;


/// Scene node descriptor-constructor                                         
///   @param desc - the node descriptor                                       
SceneTriangles::SceneTriangles(const Descriptor& desc)
   : Node {MetaOf<SceneTriangles>(), desc} {}

/// For logging                                                               
SceneTriangles::operator Debug() const {
   Code result;
   result += Node::DebugBegin();
      //result += pcSerialize<Debug>(mGeometry);
   result += Node::DebugEnd();
   return result;
}

/// Generate code for triangles from geometry                                 
///   @param define - [in/out] code definitions                               
///   @param sceneTriangles - [out] triangle code goes here                   
///   @param triangleCount - [in/out] keeps track of generated triangles      
void SceneTriangles::GenerateCode(GLSL& define, GLSL& sceneTriangles, pcptr& triangleCount) {
   if (!mGeometry->IsGenerated()) {
      // By default, geometry doesn't generate vertex positions         
      // and rasterizer requires it, so we create them                  
      // We generate normals and texture coordinates while we're at it  
      auto additional = Any::Wrap(
         DataID::Of<ATriangle>,
         DataID::Of<Point3>,
         DataID::Of<Normal>,
         DataID::Of<Sampler2>
      );

      Any unusedSideproducts;
      Any geometryBlock { mGeometry->GetBlock() };
      Verb::DefaultCreateInner(geometryBlock, additional, unusedSideproducts);
      mGeometry->Generate();
   }

   if (!mGeometry->CheckTopology<ATriangle>())
      throw Except::Content(pcLogSelfError
         << "Incompatible topology for: " << mGeometry);

   // Triangles                                                         
   static const GLSL triangleStruct =
      "struct Triangle {\n"
      "   vec3 a; vec2 aUV;\n"
      "   vec3 b; vec2 bUV;\n"
      "   vec3 c; vec2 cUV;\n"
      "   vec3 n;\n"
      "};\n\n";

   if (!define.Find(triangleStruct))
      define += triangleStruct;

   // Aggregate all triangles in an array of sorts:                     
   // const Triangle cTriangles[N] = Triangle[N](                       
   //      Triangle(a, aUV, b, bUV, c, cUV, n),                         
   //      ... N times                                                  
   //   );                                                              
   const auto count = mGeometry->GetTriangleCount();
   for (pcptr i = 0; i < count; ++i) {
      auto position = mGeometry->GetTriangleTrait<Traits::Position>(i);
      if (position.IsEmpty())
         throw Except::Content(pcLogSelfError
            << "Can't rasterize a triangle without Traits::Position");

      if (!position.InterpretsAs<vec3>(1))
         TODO();

      auto normal = mGeometry->GetTriangleTrait<Traits::Aim>(i);
      if (normal.IsEmpty())
         throw Except::Content(pcLogSelfError
            << "Can't rasterize a triangle without Traits::Aim");

      if (!normal.InterpretsAs<vec3>(1))
         TODO();

      auto texture = mGeometry->GetTriangleTrait<Traits::Sampler>(i);
      if (texture.IsEmpty())
         throw Except::Content(pcLogSelfError
            << "Can't rasterize a triangle without Traits::Sampler");

      if (!texture.InterpretsAs<vec2>(1))
         TODO();

      static const GLSL separator = ", ";
      if (triangleCount > 0) {
         sceneTriangles += separator;
         sceneTriangles += "\n";
      }

      sceneTriangles += "   Triangle(";
      sceneTriangles += position.As<vec3>(0);
      sceneTriangles += ", ";
      sceneTriangles += texture.As<vec2>(0);
      sceneTriangles += ", ";
      sceneTriangles += position.As<vec3>(1);
      sceneTriangles += ", ";
      sceneTriangles += texture.As<vec2>(1);
      sceneTriangles += ", ";
      sceneTriangles += position.As<vec3>(2);
      sceneTriangles += ", ";
      sceneTriangles += texture.As<vec2>(2);
      sceneTriangles += ", ";
      sceneTriangles += normal.As<vec3>(0);
      sceneTriangles += ")";
      ++triangleCount;
   }
}

/// Generate the shader stages                                                
void SceneTriangles::Generate() {
   VERBOSE_NODE("Generating code...");
   Descend();
   Consume();
}
