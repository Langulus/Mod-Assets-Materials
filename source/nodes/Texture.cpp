///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "../MContent.hpp"

/// Material node for combining and animating textures                        
///   @param parent - the parent node                                          
///   @param verb - the texturizer creator verb                                 
MaterialNodeTexture::MaterialNodeTexture(MaterialNode* parent, const Verb& verb)
   : MaterialNode {MetaData::Of<MaterialNodeTexture>(), parent, verb} {}

/// For logging                                                               
MaterialNodeTexture::operator Debug() const {
   GASM result;
   result += MaterialNode::DebugBegin();
      //result += mKeyframes;
   result += MaterialNode::DebugEnd();
   return result;
}

/// Texturizer, applying solid colors, color maps, normal maps, etc.            
///   @param verb - the texturization verb                                    
void MaterialNodeTexture::Texturize(Verb& verb) {
   PC_VERBOSE_MATERIAL("Texturizing: " << verb);
   pcptr textureId = 0;
   bool usingTextureId = false;
   bool relative = false;
   verb.GetArgument().ForEachDeep([&](const Block& group) {
      EitherDoThis
      group.ForEach([&](const Trait& trait) {
         if (trait.TraitIs<Traits::Texture>()) {
            // Texture id                                                
            PC_VERBOSE_MATERIAL("Configuring texturizer: " << trait);
            textureId = trait.AsCast<pcptr>();
            usingTextureId = true;
         }
         else if (trait.TraitIs<Traits::Relative>()) {
            // Relativity                                                
            PC_VERBOSE_MATERIAL("Configuring texturizer: " << trait);
            relative = trait.AsCast<bool>();
         }
      })
      OrThis
      group.ForEach([&](const real& id) {
         // Texture id                                                   
         PC_VERBOSE_MATERIAL("Configuring texturizer: " << id);
         textureId = static_cast<pcptr>(id);
         usingTextureId = true;
      });
   });

   const auto time = PCTime::FromSeconds(verb.GetTime());
   KeyframeMap* frameMap = nullptr;
   if (usingTextureId) {
      // Check if keyframe channel exists                                 
      if (mKeyframes.FindKey(textureId) == uiNone)
         mKeyframes.Add(textureId, {});

      frameMap = &mKeyframes[textureId];
   }
   else {
      // Or use the global keyframes if no channel specified            
      frameMap = &mKeyframesGlobal;
   }

   if (frameMap->FindKey(time) == uiNone) {
      // No such keyframe exists, so create it                           
      PC_VERBOSE_MATERIAL("Texture #" << textureId << " keyframe added at "
         << time << ": " << verb);
      frameMap->Add(time, verb);
      verb << this;
      return;
   }

   // Combine the keyframes' verbs                                       
   if (relative)
      (*frameMap)[time].GetArgument() << verb.GetArgument();
   else
      (*frameMap)[time] = verb;

   verb << this;
}

/// Retrieves sampler coordinates from the node environment                     
/// May either return a local 'uv' symbol, or a uniform/varying               
///   @return the texture coordinate symbol                                    
GLSL MaterialNodeTexture::GetTextureCoordinates() {
   auto sampler = GetValue(Traits::Sampler::Reflect(), nullptr, GetRate(), false);
   if (!sampler.IsValid())
      throw Except::Content(pcLogSelfError
         << "No texture coordinates available");
   return sampler.GetOutputSymbolAs(MetaData::Of<vec2f>(), 0);
}

/// Assembles a GLSL texture(...) function                                    
///   @param textureSymbol - the name of the sampler                           
///   @param meta - the format of the texture                                 
///   @return the resulting texture(...) code                                 
GLSL GetPixel(const GLSL& textureSymbol, const GLSL& uvSymbol, DMeta meta) {
   if (!meta)
      throw Except::Content(pcLogError << "Unknown texture format");

   GLSL symbol;
   const auto channels = meta->GetMemberCount();
   switch (channels) {
   case 1:
      return "texture(" + textureSymbol + ", " + uvSymbol + ").rrrr";
   case 2:
      return "texture(" + textureSymbol + ", " + uvSymbol + ").rgrg";
   case 3: case 4:
      return "texture(" + textureSymbol + ", " + uvSymbol + ")";
   default:
      throw Except::Content(pcLogError
         << "Unsupported texture format: " << meta);
   }
}

/// Turn a keyframe and all of its dependencies to GLSL code                  
///   @param frameMap - the texture channel keyframes                           
///   @param keyIdx - the index of the keyframe                                 
///   @param uv - the texture coordinate symbol                                 
///   @return the generated GLSL code                                          
GLSL MaterialNodeTexture::GetKeyframe(KeyframeMap* frameMap, pcptr keyIdx, const GLSL& uv) {
   // Scan the keyframe arguments                                       
   GLSL symbol;
   auto& keyframe = frameMap->GetValue(keyIdx);
   PC_VERBOSE_MATERIAL("Analyzing keyframe #" << keyIdx << ": " << keyframe);

   // Scan the keyframe verb                                             
   bool usingChannelId = false;
   pcptr channelId = 0;
   keyframe.GetArgument().ForEachDeep([&](const Block& group) {
      EitherDoThis
      group.ForEach([&](const real& id) {
         // Get a channel ID                                             
         usingChannelId = true;
         channelId = static_cast<pcptr>(id);
      })
      OrThis
      group.ForEach([&](const GASM& code) {
         // Generate keyframe from GASM code                              
         auto uvNode = MaterialNodeValue::Local(
            this, Trait::From<Traits::Sampler, vec2>(), mRate, 
            uv.IsEmpty() ? GetTextureCoordinates() : uv
         );
         uvNode.DoGASM(code);
         symbol = uvNode.GetOutputSymbolAs(MetaData::Of<vec4f>(), 0);
      })
      OrThis
      group.ForEach([&](const rgba& color) {
         // Generate keyframe from a static color                        
         symbol += color;
      })
      OrThis
      group.ForEach([&](const Construct& construct) {
         bool relevantConstruct = false;
         if (construct.InterpretsAs<AFile>()) {
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

         if (!relevantConstruct) {
            throw Except::Content(pcLogSelfError 
               << "Irrelevant keyframe construct: " << construct
            );
         }
      });
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

   if (symbol.IsEmpty())
      throw Except::Content(pcLogSelfError << "Bad keyframe symbol");
   return symbol;
}

/// Generate sampler definitions                                                
///   @param frameMap - the keyframe map to generate                           
///   @param uv - the texture coordinate symbol                                 
///   @return the generated GLSL code                                          
GLSL MaterialNodeTexture::GenerateDefinition(KeyframeMap* frameMap, const GLSL& uv) {
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
   for (pcptr i = 1; i < count; ++i) {
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
void MaterialNodeTexture::Generate() {
   PC_VERBOSE_MATERIAL("Generating code...");
   Descend();
   Consume();

   pcptr totalKeyframeCount = 0;
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

      Commit(ShaderToken::Texturize, 
         "vec4 texturized = " + code + ";\n");
      Expose<Traits::Color, vec4>("texturized");
      return;
   }

   // If this is reached, then we have a complex scenario of multiple   
   // channels and/or keyframes                                          
   // Combine all these in a single Texturize() function                  
   const bool hasGlobalKeyframes = !mKeyframesGlobal.IsEmpty();
   GLSL texturize = "vec4 Texturize(int id, in vec2 uv) {\n";
   if (hasGlobalKeyframes)
      texturize += "   vec4 temporary = " + GenerateDefinition(&mKeyframesGlobal, "uv") + ";\n";

   for (pcptr i = 0; i < mKeyframes.GetCount(); ++i) {
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
   if (channel.IsEmpty())
      throw Except::Content(pcLogSelfError
         << "No texture channel available");

   const GLSL define = "vec4 texturized = Texturize("
      + channel + ", " + uv + ");\n\n";
   Commit(ShaderToken::Texturize, define);
   Expose<Traits::Color, vec4>("texturized");
}
