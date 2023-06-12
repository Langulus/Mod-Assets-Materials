///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Texture.hpp"

using namespace Nodes;


/// Get pixel from sampler                                                    
///   @param {0} - sampler name                                               
///   @param {1} - texture coordinates                                        
constexpr Token GetPixelFunction = R"shader(
   texture({0}, {1})
)shader";

/// Texture flow function                                                     
///   @param {0} - first keyframe code                                        
///   @param {1} - intermediate keyframe branches                             
///   @param {2} - last keyframe code                                         
constexpr Token TextureFlowFunction = R"shader(
   vec4 TextureFlow(in float startTime, in float time, in vec2 uv) {{
      if (time <= startTime) {{
         // Time before/at start returns first keyframe
         return {0};
      }}
      {1}
      else {{
         // Time at/after last keyframe returns last keyframe
         return {2};
      }}
   }}
)shader";

/// Keyframe without texture transition                                       
///   @param {0} - frame end time                                             
///   @param {1} - GetPixelFunction function call                             
constexpr Token TextureFlowBranch = R"shader(
   else if (time < {0}) {{
      return {1};
   }}
)shader";

/// Keyframe with transition                                                  
///   @param {0} - frame end time                                             
///   @param {1} - frame start time                                           
///   @param {2} - frame length (end - start)                                 
///   @param {3} - GetPixelFunction function call from start side             
///   @param {4} - GetPixelFunction function call from end side               
constexpr Token TextureFlowBranchMix = R"shader(
   else if (time < {0}) {{
      const float ratio = (time - {1}) / {2};
      return mix({3}, {4}, ratio);
   }}
)shader";


/// Texture node descriptor-constructor                                       
///   @param desc - the node descriptor                                       
Texture::Texture(const Descriptor& desc)
   : Node {MetaOf<Texture>(), desc} {
   // Extract texture id                                                
   mDescriptor.ExtractTrait<Traits::Texture>(mTextureId);
   mDescriptor.ExtractDataAs(mTextureId);

   // Extract animation keyframes                                       
   mKeyframes.Push(mDescriptor.mVerbs);
}

/// For logging                                                               
Texture::operator Debug() const {
   Code result;
   result += Node::DebugBegin();
      //result += mKeyframes;
   result += Node::DebugEnd();
   return result;
}

/// Retrieves sampler coordinates from the node environment                   
/// May either return a local 'uv' symbol, or a uniform/varying               
///   @return the texture coordinate symbol                                   
GLSL Texture::GetTextureCoordinates() {
   auto sampler = GetValue(MetaTrait::Of<Traits::Sampler>(), nullptr, GetRate(), false);
   LANGULUS_ASSERT(sampler.IsValid(), Material, "No texture coordinates available");
   return sampler.GetOutputSymbolAs(MetaData::Of<Vec2f>(), 0);
}

/// Assembles a GLSL texture(...) function                                    
///   @param sampler - sampler token                                          
///   @param uv - texture coordinates                                         
///   @param result - the resulting color format                              
///   @return generated texture call                                          
GLSL GetPixel(const GLSL& sampler, const GLSL& uv, DMeta result) {
   LANGULUS_ASSERT(meta, Material, "Unknown texture format");
   const auto pixel = TemplateFill(GetPixelFunction, sampler, uv);
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
   // Scan the keyframe arguments                                       
   GLSL symbol;
   auto& keyframe = frameMap->GetValue(keyIdx);
   VERBOSE_NODE("Analyzing keyframe #", keyIdx, ": ", keyframe);

   // Scan the keyframe verb                                            
   bool usingChannelId {};
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
               auto creator = Verb::From<Verbs::Create>({}, &construct);
               Any environment = mProducer->GetOwners();
               Verb::ExecuteVerb(environment, creator);
               creator.GetOutput().ForEachDeep([&](CGeneratorTexture* t) {
                  t->Generate();
                  auto uniform = GetProducer()->AddInput(
                     RRate::PerAuto, Trait::From<Traits::Texture>(t), true);
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
   }

   LANGULUS_ASSERT(!symbol.IsEmpty(), Material, "Bad keyframe symbol");
   return symbol;
}

/// Generate sampler definitions                                              
///   @param frameMap - the keyframe map to generate                          
///   @param uv - the texture coordinate symbol                               
///   @return the generated GLSL code                                         
GLSL Texture::GenerateDefinition(KeyframeMap* frameMap, const GLSL& uv) {
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
}

/// Generate the shader stages                                                
Symbol Texture::Generate() {
   Descend();

   Count totalKeyframeCount {};
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
   Expose<Traits::Color, Vec4>("texturized");
}
