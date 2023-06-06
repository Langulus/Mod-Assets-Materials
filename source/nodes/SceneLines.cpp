///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "SceneLines.hpp"

using namespace Nodes;


/// Scene node as member constructor                                          
///   @param parent - the owning node                                         
///   @param desc - the node descriptor                                       
SceneLines::SceneLines(Node* parent, const Descriptor& desc)
   : Node {MetaOf<SceneLines>(), parent, desc} {}

/// Scene node descriptor-constructor                                         
///   @param desc - the node descriptor                                       
SceneLines::SceneLines(const Descriptor& desc)
   : Node {MetaOf<SceneLines>(), desc} {}

/// For logging                                                               
SceneLines::operator Debug() const {
   Code result;
   result += Node::DebugBegin();
      //result += pcSerialize<Debug>(mGeometry);
   result += Node::DebugEnd();
   return result;
}

/// Generate scene code                                                       
///   @return the array of lines symbol                                       
Symbol SceneLines::Generate() {
   Descend();

   if (!mGeometry->IsGenerated()) {
      // By default, geometry doesn't generate vertex positions         
      // and rasterizer requires it, so we create them                  
      auto additional = Any::Wrap(
         DataID::Of<ALine>,
         DataID::Of<Point3>,
         DataID::Of<Sampler1>
      );

      Any unusedSideproducts;
      Any geometryBlock { mGeometry->GetBlock() };
      Verb::DefaultCreateInner(geometryBlock, additional, unusedSideproducts);
      mGeometry->Generate();
   }

   if (!mGeometry->CheckTopology<ALine>())
      throw Except::Content(pcLogSelfError
         << "Incompatible topology for: " << mGeometry);

   // Lines                                                             
   static const GLSL lineStruct =
      "struct Line {\n"
      "   vec3 a; vec2 aUV;\n"
      "   vec3 b; vec2 bUV;\n"
      "};\n\n";

   if (!define.Find(lineStruct))
      define += lineStruct;

   // Aggregate all lines in an array of sorts:                         
   // const Line cLines[N] = Line[N](                                   
   //      Line(a, aUV, b, bUV),                                        
   //      ... N times                                                  
   //   );                                                              
   const auto count = mGeometry->GetLineCount();
   for (pcptr i = 0; i < count; ++i) {
      auto position = mGeometry->GetLineTrait<Traits::Position>(i);
      auto texture = mGeometry->GetLineTrait<Traits::Sampler>(i);

      if (!position.InterpretsAs<vec3>(1))
         TODO();
      if (!texture.InterpretsAs<vec2>(1))
         TODO();

      static const GLSL separator = ", ";

      if (lineCount > 0) {
         sceneLines += separator;
         sceneLines += "\n";
      }

      sceneLines += "   Line(";
      sceneLines += position.As<vec3>(0);
      sceneLines += ", ";
      sceneLines += texture.As<vec2>(0);
      sceneLines += ", ";
      sceneLines += position.As<vec3>(1);
      sceneLines += ", ";
      sceneLines += texture.As<vec2>(1);
      sceneLines += ")";

      ++lineCount;
   }
}