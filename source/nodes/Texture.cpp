///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Texture.hpp"
#include "../Material.hpp"
#include <Math/Colors.hpp>

using namespace Nodes;


/// Texture node descriptor-constructor                                       
///   @param desc - the node descriptor                                       
Texture::Texture(const Descriptor& desc)
   : Node {MetaOf<Texture>(), desc} {
   // Create any subnodes here, it is allowed                           
   // This will also execute any encountered [subcode], and set rate    
   InnerCreate();

   // Extract texture id, decides to which texture slot to bind         
   if (mDescriptor.ExtractTrait<Traits::Texture>(mTextureId))
      VERBOSE_NODE("Texture id changed to: ", mTextureId);

   // Extract Traits::File, if any                                      
   Any file;
   mDescriptor.ExtractTrait<Traits::File>(file);
   if (!file.IsEmpty()) {
      mTexture = CreateTexture(file);
      VERBOSE_NODE("Texture generator changed to: ", mTexture);
   }
   
   for (const auto pair : mDescriptor.mConstructs) {
      // Create texture generators from sub-constructs                  
      if (!pair.mKey->template CastsTo<A::Texture>() &&
          !pair.mKey->template CastsTo<A::File>()) {
         Logger::Warning(Self(), "Ignored constructs of type ", pair.mKey);
         continue;
      }

      for (auto& construct : pair.mValue) {
         mTexture = CreateTexture(construct);
         VERBOSE_NODE("Texture generator changed to: ", mTexture);
      }
   }
   
   // Consider all provided data                                        
   for (const auto pair : mDescriptor.mAnythingElse) {
      if (pair.mKey->template CastsTo<A::Texture>()) {
         // Reuse a texture generator directly                          
         mTexture = pair.mValue.Last().As<A::Texture*>();
         VERBOSE_NODE("Texture generator changed to: ", mTexture);
      }
      else if (pair.mKey->Is<Text>()) {
         // Any other text is considered a texture filename             
         for (auto& filename : pair.mValue) {
            mTexture = CreateTexture(filename);
            VERBOSE_NODE("Texture generator changed to: ", mTexture);
         }
      }
      else if (pair.mKey->template CastsTo<A::Number>()) {
         // Set texture id                                              
         mTextureId = pair.mValue.Last().AsCast<Index>();
         VERBOSE_NODE("Texture id changed to: ", mTextureId);
      }
      else {
         Logger::Warning(Self(), "Ignored data of type ", pair.mKey);
         continue;
      }
   }
}

/// A snippet that forward a descriptor to a creation verb in the hierarchy   
///   @param descriptor - the descriptor for the texture                      
///   @return the produced texture                                            
Ptr<A::Texture> Texture::CreateTexture(const Any& descriptor) {
   Verbs::Create creator {Construct::From<A::Texture>(descriptor)};
   if (mMaterial->DoInHierarchy<Seek::Above>(creator))
      return creator.GetOutput().As<A::Texture*>();

   Logger::Error(Self(), "Couldn't create texture from: ", descriptor);
   return {};
}

/// Assembles a GLSL texture(...) function                                    
///   @param sampler - sampler token                                          
///   @param uv - texture coordinates                                         
///   @param result - the resulting color format                              
///   @return generated texture call                                          
GLSL GetPixel(const GLSL& sampler, const GLSL& uv, DMeta result) {
   LANGULUS_ASSERT(result, Material, "Unknown texture format");
   const auto pixel = Text::TemplateRt(GetPixelFunction, sampler, uv);
   switch (result->GetMemberCount()) {
   case 1:           return pixel + ".rrrr";
   case 2:           return pixel + ".rgrg";
   case 3: case 4:   return pixel;
   default: LANGULUS_THROW(Material, "Unsupported texture format");
   }
}

/// Turn a keyframe and all of its dependencies to GLSL code                  
///   @param frameMap - the texture channel keyframes                         
///   @param keyIdx - the index of the keyframe                               
///   @param uv - the texture coordinate symbol                               
///   @return the generated GLSL code                                         
GLSL Texture::GenerateKeyframe(const Temporal& keyframe) {
   GLSL symbol;

   // Scan the keyframe verb                                            
   /*bool usingChannelId {};
   Offset channelId {};
   keyframe.ForEachDeep([&](const Block& group) {
      group.ForEach(
         [&](const Real& id) {
            // Get a channel ID                                         
            usingChannelId = true;
            channelId = static_cast<Offset>(id);
         },
         [&](const Code& code) {
            // Generate keyframe from GASM code                         
            auto uvNode = Nodes::Value::Local(
               this, Trait::From<Traits::Sampler, Vec2>(), mRate, 
               uv.IsEmpty() ? GetTextureCoordinates() : uv
            );
            uvNode.DoGASM(code);
            symbol = uvNode.GetOutputSymbolAs(MetaData::Of<Vec4f>(), 0);
         },
         [&](const RGBA& color) {
            // Generate keyframe from a static color                    
            symbol += color;
         },
         [&](const Construct& construct) {
            bool relevantConstruct = false;
            if (construct.CastsTo<A::File>()) {
               // Generate keyframe from a texture file                 
               Verbs::Create creator {&construct};
               Any environment = mProducer->GetOwners();
               Verb::ExecuteVerb(environment, creator);
               creator.GetOutput().ForEachDeep([&](A::Texture* t) {
                  t->Generate();
                  auto uniform = GetProducer()->AddInput(Rate::Auto, Trait::From<Traits::Texture>(t), true);
                  symbol = GetPixel(
                     uniform,
                     uv.IsEmpty() ? GetTextureCoordinates() : uv,
                     t->GetFormat()
                  );
                  relevantConstruct = true;
               });
            }
            else {
               //TODO support other kinds of constructs?
               TODO();
            }

            LANGULUS_ASSERT(relevantConstruct, Material, "Irrelevant keyframe construct");
         }
      );
   });

   // Fallback for when only channel ID is available                    
   if (symbol.IsEmpty() && usingChannelId) {
      const auto texture = GetValue<Traits::Texture>();
      const auto& trait = texture.GetOutputTrait();
      symbol = GetPixel(
         texture.GetOutputSymbol(), 
         uv.IsEmpty() ? GetTextureCoordinates() : uv, 
         trait.GetMeta()
      );
   }*/

   LANGULUS_ASSERT(!symbol.IsEmpty(), Material, "Bad keyframe symbol");
   return symbol;
}

/// Generate sampler definitions                                              
///   @param frameMap - the keyframe map to generate                          
///   @param uv - the texture coordinate symbol                               
///   @return the generated GLSL code                                         
/*GLSL Texture::GenerateDefinition(KeyframeMap* frameMap, const GLSL& uv) {
   if (frameMap->IsEmpty())
      return {};

   const GLSL functionId = pcToHex(frameMap);
   if (frameMap->GetCount() == 1) {
      // A single keyframe just returns itself                          
      return GetKeyframe(frameMap, 0, uv);
   }

   // Number of keyframes available                                     
   const auto count = frameMap->GetCount();
   const auto animationStart = frameMap->GetKey(0).SecondsReal();

   // The animate function picks the keyframes depending on time        
   GLSL definition = 
      "vec4 AnimateTextures_"_glsl + functionId + "(in float time, in vec2 uv) {\n"
      "   if (time <= " + animationStart + ") {\n"
      "      // Time before/at start returns first keyframe\n"
      "      return " + GetKeyframe(frameMap, 0, "uv") + ";\n"
      "   }\n";

   // Since samplers can't be saved to a variable, we have to explictly 
   // generate an if statement for each inter-keyframe situation :(     
   for (Offset i = 1; i < count; ++i) {
      const auto frameStart = frameMap->Keys()[i-1].SecondsReal();
      const auto frameEnd = frameMap->Keys()[i].SecondsReal();
      const auto frameLength = frameEnd - frameStart;
      const auto textureStart = GetKeyframe(frameMap, i - 1, "uv");
      const auto textureEnd = GetKeyframe(frameMap, i, "uv");

      if (textureStart == textureEnd) {
         // No need to mix textures                                     
         definition += 
            "   else if (time < "_glsl + frameEnd + ") {\n"
            "      return " + textureStart + ";\n"
            "   }\n";
      }
      else {
         // Need to mix two textures                                    
         definition += 
            "   else if (time < "_glsl + frameEnd + ") {\n"
            "      const float ratio = (time - " + frameStart + ") / " + frameLength + ";\n"
            "      return mix(" + textureStart + ", " + textureEnd + ", ratio);"
            "   }\n";
      }
   }

   definition +=
      "   else {\n"
      "      // Time at/after last keyframe returns last keyframe\n"
      "      return " + GetKeyframe(frameMap, count-1, "uv") + ";\n"
      "   }\n"
      "}\n\n";

   // Commit the animation function                                     
   Commit(ShaderToken::Functions, definition);

   // And return its usage                                              
   return "AnimateTextures_"_glsl + functionId + "(time, " 
      + (uv.IsEmpty() ? GetTextureCoordinates() : uv) + ")";
}*/

/// Generate the shader stages                                                
const Symbol& Texture::Generate() {
   Descend();

   /*Count totalKeyframeCount {};
   totalKeyframeCount += mKeyframesGlobal.GetCount();
   for (auto& channel : mKeyframes)
      totalKeyframeCount += channel.GetCount();

   if (totalKeyframeCount == 1) {
      // If total count of keyframes is 1, just return the keyframe     
      // There's no need of mixing functions                            
      // Also, uv symbol is passed empty, because it will be requested  
      // only on demand - we might not require it at all                
      GLSL code;
      for (auto& channel : mKeyframes)
         code += GenerateDefinition(&channel, "");
      code += GenerateDefinition(&mKeyframesGlobal, "");

      Commit(ShaderToken::Texturize, "vec4 texturized = " + code + ";\n");
      Expose<Traits::Color, Vec4>("texturized");
      return;
   }

   // If this is reached, then we have a complex scenario of multiple   
   // channels and/or keyframes                                         
   // Combine all these in a single Texturize() function                
   const bool hasGlobalKeyframes = !mKeyframesGlobal.IsEmpty();
   GLSL texturize = "vec4 Texturize(int id, in vec2 uv) {\n";
   if (hasGlobalKeyframes)
      texturize += "   vec4 temporary = " + GenerateDefinition(&mKeyframesGlobal, "uv") + ";\n";

   for (Offset i = 0; i < mKeyframes.GetCount(); ++i) {
      // Generate a branch for each channel                             
      const auto& id = mKeyframes.GetKey(i);
      if (id != mKeyframes.Keys()[0])
         texturize += "   else \n";

      if (id != mKeyframes.Keys().Last())
         texturize += "   if (id == "_glsl + id + ")\n";

      if (hasGlobalKeyframes)
         texturize += "      temporary += ";
      else
         texturize += "      return ";

      texturize += GenerateDefinition(&mKeyframes.GetValue(i), "uv") + ";\n";
   }

   // Close up the function                                             
   if (hasGlobalKeyframes)
      texturize += "   return temporary;\n";
   texturize += "}\n\n";

   Commit(ShaderToken::Functions, texturize);

   //TODO maybe check if uv is required at all?
   const auto uv = GetTextureCoordinates();
   if (mKeyframes.GetCount() == 1) {
      // A single texture channel                                       
      const GLSL define = "vec4 texturized = Texturize(" + uv + ");\n\n";
      Commit(ShaderToken::Texturize, define);
      Expose<Traits::Color, vec4>("texturized");
      return;
   }

   // Multiple texture channels                                         
   auto channel = GetSymbol<Traits::Texture>(GetRate(), false);
   LANGULUS_ASSERT(!channel.IsEmpty(), Material, "No texture channel available")
   const GLSL define = "vec4 texturized = Texturize(" + channel + ", " + uv + ");\n\n";
   Commit(ShaderToken::Texturize, define);
   Expose<Traits::Color, Vec4>("texturized");*/
   TODO();
}
