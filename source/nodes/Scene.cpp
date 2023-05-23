///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "../MContent.hpp"

/// Material node from geometry generator                                       
///   @param parent - the parent node                                          
///   @param verb - the scene creator verb                                    
MaterialNodeScene::MaterialNodeScene(MaterialNode* parent, const Verb& verb)
   : MaterialNode{ MetaData::Of<MaterialNodeScene>(), parent, verb } {
   // Request the geometry generator                                    
   auto creator = Verb::From<Verbs::Create>({}, verb.GetArgument());
   GetContentManager()->Create(creator);
   creator.GetOutput().ForEachDeep([this](CGeneratorGeometry* g) {
      mGeometry = g;
   });
}

/// For logging                                                               
MaterialNodeScene::operator Debug() const {
   GASM result;
   result += MaterialNode::DebugBegin();
      result += pcSerialize<Debug>(mGeometry);
   result += MaterialNode::DebugEnd();
   return result;
}

/// Move/rotate/scale the scene                                                
///   @param verb - the move verb                                             
void MaterialNodeScene::Move(Verb& verb) {
   if (verb.GetArgument().IsEmpty())
      return;

   // Add/reuse a transform child node and relay the verb to it         
   auto transformer = EmplaceChildUnique(MaterialNodeTransform(this, verb));
   transformer->Move(verb);
}

/// Texturize scene, applying solid colors, color maps, normal   maps, etc.      
///   @param verb - the texturization verb                                    
void MaterialNodeScene::Texturize(Verb& verb) {
   if (verb.GetArgument().IsEmpty())
      return;

   // Add/reuse a texturizer child node and relay the verb to it         
   auto texturizer = EmplaceChildUnique(MaterialNodeTexture(this, verb));
   texturizer->Texturize(verb);
}

/// Generate code for signed distance field from geometry                     
///   @param define - [in/out] code definitions                                 
///   @param scene - [out] SDF code goes here                                 
void MaterialNodeScene::GenerateSDFCode(GLSL& define, GLSL& scene) {
   if (!mGeometry->IsGenerated()) {
      // By default, geometry doesn't generate GLSL code                  
      // and raymarcher requires it, so we create them                  
      auto additional = Any::Wrap(
         DataID::Of<ATriangle>,
         DataID::Of<GLSL>
      );

      Any unusedSideproducts;
      Any geometryBlock { mGeometry->GetBlock() };
      Verb::DefaultCreateInner(geometryBlock, additional, unusedSideproducts);
      mGeometry->Generate();
   }

   // Get the SDF code from the generated content                        
   const auto codeTrait = mGeometry->GetData<Traits::Code>();
   if (!codeTrait || codeTrait->IsEmpty())
      throw Except::Content(pcLogSelfError 
         << "No SDF available for " << mGeometry);

   // Add definition if not added yet                                    
   auto& definitionCode = codeTrait->As<GLSL>(0);
   if (!define.Find(definitionCode))
      define += definitionCode;

   auto& usageCode = codeTrait->As<GLSL>(1);
   if (scene.IsEmpty()) {
      // First scene entry                                                
      scene += usageCode;
      return;
   }

   // Define the union operation if not yet defined                     
   static const GLSL unionFunction =
      "float opUnion(float d1, float d2) {\n"
      "   return min(d1,d2);\n"
      "}\n\n";

   if (!define.Find(unionFunction))
      define += unionFunction;

   // Then union the SDF to the rest of the SceneSDF()                  
   scene = "opUnion(" + scene;
   scene += ", ";
   scene += usageCode;
   scene += ")";
}

/// Generate code for triangles from geometry                                 
///   @param define - [in/out] code definitions                                 
///   @param sceneTriangles - [out] triangle code goes here                     
///   @param triangleCount - [in/out] keeps track of generated triangles      
void MaterialNodeScene::GenerateTriangleCode(GLSL& define, GLSL& sceneTriangles, pcptr& triangleCount) {
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

/// Generate code for lines from geometry                                       
///   @param define - [in/out] code definitions                                 
///   @param sceneLines - [out] line code goes here                           
///   @param lineCount - [in/out] keeps track of generated lines               
void MaterialNodeScene::GenerateLineCode(GLSL& define, GLSL& sceneLines, pcptr& lineCount) {
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

/// Generate the shader stages                                                
void MaterialNodeScene::Generate() {
   PC_VERBOSE_MATERIAL("Generating code...");
   Descend();
   Consume();
}
