///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
///                                                                           
#include "Material.hpp"
#include "MaterialLibrary.hpp"
#include <Langulus/Edit.hpp>


/// Material construction                                                     
///   @param producer - the producer                                          
///   @param desc - instructions for configuring the material                 
Material::Material(Things::AssetModule* producer, Many const& desc)
   : Resolvable   {this}
   , ProducedFrom {producer, desc}
   , mRoot        {this, desc} {
   Logger::Verbose(Self(), "Initializing...");

   // Extract default rate if any                                       
   if (not desc.ExtractTag<Tags::Rate>(mDefaultRate))
      desc.ExtractData(mDefaultRate);

   // Scan descriptor for Tags::Input and Tags::Output              
   desc.ForEachDeep([&](Tag const& tag) {
      auto commonRate = Rate::Auto;

      tag.ForEachDeep([&](Many const& part) {
         part.ForEach(
            [&](RefreshRate  i) noexcept { commonRate = i; },
            [&](Tag const& i) noexcept {
               // Add material input/output                             
               if (tag.IsTrait<Tags::Input>())
                  AddInput(commonRate, i, true);
               else if (tag.IsTrait<Tags::Output>())
                  AddOutput(commonRate, i, true);
               else
                  TODO(); // Add constants?
            }
         );
      });
   });

   mRoot.Dump();
   Logger::Verbose(Self(), "Initialized");
}

/// Create nodes inside the material                                          
///   @param verb - creation verb                                             
Material::~Material() {
   Logger::Verbose(Self(), "Destroying...");
   mDescriptor.Reset();
}

/// Create nodes inside the material                                          
///   @param verb - creation verb                                             
void Material::Create(Verb& verb) {
   mRoot.Create(verb);
}

/// Generate shaders                                                          
///   @param tag - the tag to generate                                    
///   @param index - tag group to generate                                  
///   @return true if data was generated                                      
bool Material::Generate(TMeta tag, size_t) {
   const auto found = mDataListMap.FindIt(tag);
   if (found)
      return true;

   // Reserve shader stages inside the Asset's data list map, in the    
   // Tags::Shader bucket. This marks that this material has been     
   // generated from now on, even if a specific stage doesn't exist.    
   mDataListMap.Insert(tag);
   mDataListMap[tag].New(ShaderStage::Counter, GLSL {});

   // If a vertex shader is missing, add a default one                  
   GLSL& vs = GetStage(ShaderStage::Vertex);
   if (vs.IsEmpty()) {
      // Default vertex stage - a rectangle filling the screen          
      Commit(Rate::Vertex, ShaderToken::Transform,
         "const vec2 outUV = vec2(gl_VertexIndex & 2, (gl_VertexIndex << 1) & 2);\n"
         "gl_Position = vec4(outUV * 2.0f - 1.0f, 0.0f, 1.0f);\n"
      );
   }

   // Generate inputs and outputs where needed                          
   GenerateInputs();
   GenerateOutputs();
   GenerateUniforms();

   // Finish all the stages, by writing shader versions to all of them, 
   // and other final touches                                           
   ForEachStage([&](Stage stage) {
      if (stage.code.IsEmpty())
         return;

      // Write shader version to all relevant codes                     
      stage.code.SetVersion("450");

      Logger::Verbose(Self(), "Stage (", ShaderStage::Names[stage.id], "):\n");
      Logger::Verbose(Self(), stage.code.Pretty());
   });

   return true;
}

/// Get material adapter for lower or higher level of detail                  
///   @param lod - the level-of-detail state                                  
///   @return a pointer to the material generator                             
auto Material::GetLOD(const LOD&) const -> Ref<Things::Material> {
   TODO();
   return {};
}

/// Get default material rate                                                 
///   @return the rate                                                        
auto Material::GetDefaultRate() const noexcept -> RefreshRate {
   return mDefaultRate;
}

/// Commit a code snippet to a specific stage and place                       
///	@param stage - the shader stage to commit to                            
///   @param place - the shader token to commit changes at                    
///   @param addition - the code to commit                                    
void Material::Commit(RefreshRate rate, Token const& place, Token const& addition) {
   const auto stage = rate.GetStageIndex();
   auto& code = GetStage(stage);
   if (not code) {
      code += GLSL::Template(stage);
      VERBOSE_NODE("Added default template for ", ShaderStage::Names[stage]);
   }

   Edit(code).Select(place) >> addition;
   VERBOSE_NODE("Added code: ");
   VERBOSE_NODE(GLSL {addition}.Pretty());
}

/// Get a GLSL stage                                                          
///   @param stage - the stage index                                          
///   @return the code associated with the stage                              
GLSL& Material::GetStage(size_t stage) {
   auto stages = GetDataList<Tags::Shader>();
   LglsAssumeDev(stages,
      "No data inside material");
   LglsAssumeDev(stage < ShaderStage::Counter,
      "Bad stage offset");
   LglsAssumeDev(stages->GetCount() == ShaderStage::Counter,
      "Bad material data count");
   IF_SAFE(for (auto& s : *stages) {
      LglsAssumeDev(s.IsExact<GLSL>() and s.GetCount() == 1,
         "Bad stage commited");
   })

   return (*stages)[stage].Get<GLSL>();
}

/// Get a GLSL stage (const)                                                  
///   @param stage - the stage index                                          
///   @return the code associated with the stage                              
const GLSL& Material::GetStage(size_t stage) const {
   return const_cast<Material*>(this)->GetStage(stage);
}

/// Execute a function for each stage                                         
///   @param call - the lambda function to call (use GLSL for argument)       
void Material::ForEachStage(auto&& call) {
   using F = Deref<decltype(call)>;
   using A = ArgumentOf<F>;
   static_assert(CT::Same<A, GLSL> or CT::Exact<A, Stage>,
      "Function argument must be of type GLSL");

   if constexpr (CT::Same<A, GLSL>) {
      for (size_t i = 0; i < ShaderStage::Counter; ++i)
         call(GetStage(i));
   }
   else {
      for (size_t i = 0; i < ShaderStage::Counter; ++i)
         call({ShaderStage::Enum(i), GetStage(i)});
   }
}

/// Add an external input tag                                               
///   @param rate - the rate at which the input will be refreshed             
///   @param t - the input to add                                             
///   @param allowDuplicates - whether multiple such tags are allowed       
///   @return the generated symbol name                                       
GLSL Material::AddInput(RefreshRate rate, Tag const& t, bool allowDuplicates) {
   // Get local rate and type, if any                                   
   DMeta type;
   t.ForEachDeep(
      [&](RefreshRate r) noexcept { rate = r; },
      [&](DMeta       r) noexcept { type = r; }
   );

   // An input must always be declared with a rate, and that rate       
   // must always be smaller or equal to the node's rate                
   // For example, you might want Time input PerPixel, but the          
   // actual uniform will be updated PerTick. So this is                
   // where we step in to override any wrongly provided rate            
   if (rate == Rate::Auto)
      rate = Node::GetDefaultTrait(t.GetTag()).mRate;
   if (not type)
      type = Node::GetDefaultTrait(t.GetTag()).mType;

   // Find any matching available inputs                                
   auto& inputs = mInputs[rate.GetInputIndex()];
   const auto proto = Tag::FromMeta(t.GetTag(), type);
   if (not allowDuplicates) {
      auto found = inputs.Find(proto);
      if (found)
         return GenerateInputName(rate, inputs[found]);
   }

   // Add the new input                                                 
   inputs << proto;
   const auto symbol = GenerateInputName(rate, proto);
   if (t.IsTrait<Tags::Image>())
      ++mConsumedSamplers;

   VERBOSE_NODE("Added input ", Logger::Cyan, proto.GetTag(),
      " as `", symbol, "` at ", rate, " of type ", type);
   return symbol;
}

/// Add an output to the material                                             
///   @param rate - the rate at which the output is refreshed                 
///   @param t - the output to add                                            
///   @param allowDuplicates - whether multiple such tags are allowed       
///   @return the generated symbol name                                       
GLSL Material::AddOutput(RefreshRate rate, Tag const& t, bool allowDuplicates) {
   // Get local rate and type, if any                                   
   DMeta type;
   t.ForEachDeep(
      [&](RefreshRate r) noexcept { rate = r; },
      [&](DMeta       r) noexcept { type = r; }
   );

   LglsAssert(rate.IsShaderStage(),
      "Can't add material outputs to rates, "
      "that don't correspond to shader stages"
   );

   if (not type)
      type = Node::GetDefaultTrait(t.GetTag()).mType;

   auto& outputs = mOutputs[rate.GetInputIndex()];
   const auto proto = Tag::FromMeta(t.GetTag(), type);
   if (not allowDuplicates) {
      auto found = outputs.Find(proto);
      if (found)
         return GenerateOutputName(rate, outputs[found]);
   }

   // Add the new output                                                
   outputs << proto;
   const auto symbol = GenerateOutputName(rate, proto);
   VERBOSE_NODE("Added output ", Logger::Cyan, proto.GetTag(),
      " as `", symbol, "` at ", rate, " of type ", type);
   return symbol;
}

/// Adds a code snippet                                                       
///   @param rate - the shader stage to place code at                         
///   @param name - the name of the definition (to check for duplicated)      
///   @param code - the code to insert                                        
void Material::AddDefine(RefreshRate rate, Token const& name, const GLSL& code) {
   const auto stageIndex = rate.GetStageIndex();
   mDefinitions[stageIndex][name] << code;
}

/// Generate input name                                                       
///   @param rate - the rate at which the input is declared                   
///   @param tag - the tag tag for the input                              
///   @return the variable name to access the input                           
GLSL Material::GenerateInputName(RefreshRate rate, Tag const& tag) const {
   if (tag.IsTrait<Tags::Image>()) {
      // Samplers are handled differently                               
      return {tag.GetTag(), mConsumedSamplers};
   }
   else if (not rate.IsUniform()) {
      // Name is for a vertex attribute or varying                      
      return {"in", tag.GetTag()};
   }

   // Uniform name inside a uniform buffer                              
   auto rateTxt = static_cast<Text>(rate);
   auto lastns = rateTxt.Find<true>(':');
   return Text::TemplateRt("Per{}.{}", rateTxt.Select(lastns + 1), tag.GetTag());
}

/// Generate output name                                                      
///   @param rate - the rate at which the output is declared                  
///   @param tag - the tag tag for the output                             
///   @return the variable name to access the output                          
GLSL Material::GenerateOutputName(RefreshRate rate, Tag const& tag) const {
   LglsAssert(rate.IsShaderStage(), "Can't have an output outside a shader stage rate");
   return {"out", tag.GetTag()};
}

/// Generate uniform buffer descriptions for all shader stages                
void Material::GenerateUniforms() {
   // Scan all uniform rates:                                           
   // Tick, Pass, Camera, Level, Renderable, Instance                   
   for (size_t i = 0; i < RefreshRate::UniformCount; ++i) {
      const RefreshRate rate = i + RefreshRate::UniformBegin;
      auto& tags = GetInputs(i);
      if (not tags)
         continue;

      // Define the rate-dedicated uniform buffer, if there's at least  
      // one tag available                                            
      //    @param {0} - set index, should be 0 for static uniforms,    
      //                 and 1 for dynamic uniforms                     
      //    @param {1} - binding index, derived from                    
      //                 Rate::GetStaticUniformIndex, and               
      //                 Rate::GetDynamicUniformIndex respectively      
      //    @param {2} - name of the uniform buffer instance, derived   
      //                 from Rate::GetToken                            
      //    @param {3} - the list of actual variables                   
      constexpr auto layout = R"shader(
         layout(set = {0}, binding = {1})
         uniform UniformBuffer{2} {{
            {3}
         }} {2};
      )shader";

      // Generate the list of uniform variables inside the buffer       
      GLSL body;
      TMany<GLSL> names;
      for (auto& tag : tags) {
         // Skip textures for now                                       
         if (tag.IsTrait<Tags::Image>())
            continue;

         LglsAssert(tag.IsTyped(), "Uniform is not typed");
         LglsAssert(tag.GetTag(), "Uniform has undefined tag");

         const GLSL type {tag.GetType()};
         const GLSL name {tag.GetTag()};
         body += Text::TemplateRt("{} {};", type, name);
         if (&tag != &tags.Last())
            body += '\n';
         names << name;
      }

      if (not names)
         continue;

      GLSL ubo;
      if (rate.IsStaticUniform()) {
         ubo = Text::TemplateRt(layout,
            0, rate.GetStaticUniformIndex(), GLSL {rate}, body
         );
      }
      else if (rate.IsDynamicUniform()) {
         ubo = Text::TemplateRt(layout,
            1, rate.GetDynamicUniformIndex(), GLSL {rate}, body
         );
      }
      else LglsError("Uniform rate neither static, nor dynamic");

      // Add the uniform buffer to each stage it is used in             
      ForEachStage([&](GLSL& code) {
         for (auto const& name : names) {
            if (not code.Find(name))
               continue;

            Edit(code).Select(ShaderToken::Uniform) << ubo;
            break;
         }
      });
   }

   // Do another scan for the textures                                  
   // Textures are always updated per renderable for now                
   // Samplers always use layout set #2 for now                         
   size_t textureNumber = 0;
   auto& tags = GetInputs(Rate::Renderable);
   for (auto& tag : tags) {
      // Skip anything BUT textures                                     
      if (not tag.IsTrait<Tags::Image>())
         continue;

      // Define each sampler using this layout                          
      //    @param {0} - binding index, each unique texture view should 
      //                 get a unique index                             
      //    @param {1} - sampler type, deduced from the tag data type 
      //    @param {2} - the tag name, extracted from tag id        
      constexpr auto layout = R"shader(
         layout(set = 2, binding = {0})
         uniform {1} {2}{0};
      )shader";

      const GLSL type {tag.GetType()};
      const GLSL name {tag.GetTag()};
      const auto ubo = Text::TemplateRt(layout, textureNumber, type, name);

      // Add texture to each stage it is used in                        
      ForEachStage([&](GLSL& code) {
         if (not code.Find(name + textureNumber))
            return;

         Edit(code).Select(ShaderToken::Uniform) << ubo;
      });

      ++textureNumber;
   }
}

/// Generate vertex attributes (aka vertex shader inputs)                     
void Material::GenerateInputs() {
   for (size_t i = 0; i < ShaderStage::Counter; ++i) {
      const RefreshRate rate = RefreshRate::StagesBegin + i;
      auto const& inputs = GetInputs(rate);
      size_t location = 0;

      //TODO make sure that correct amount of locations are used,
      //it depends on the value size: 1 location <= 4 floats
      for (auto& input : inputs) {
         auto vkt = Node::DecayToGLSLType(input.GetType());
         if (not vkt) {
            Logger::Error("Unsupported base for shader attribute ", 
               input.GetTag(), ": ", vkt, " (decayed from ", 
               input.GetType(), ")"
            );
            LANGULUS_THROW(Material,
               "Unsupported base for shader attribute");
         }

         // Format the vertex attribute                                 
         //    @param {0} - attribute location index                    
         //    @param {1} - type of the vertex attribute                
         //    @param {2} - name of the vertex attribute                
         constexpr auto layout = R"shader(
            layout(location = {0})
            in {1} {2};
         )shader";

         const GLSL type {vkt};
         const GLSL name {GenerateInputName(rate, input)};
         const auto definition = Text::TemplateRt(layout, location, type, name);

         // Add input to code                                           
         Commit(rate, ShaderToken::Input, definition);
         ++location;
      }
   }
}

/// Generate shader outputs                                                   
void Material::GenerateOutputs() {
   for (size_t i = 0; i < ShaderStage::Counter; ++i) {
      const RefreshRate rate = RefreshRate::StagesBegin + i;
      auto const& outputs = GetOutputs(rate);
      size_t location = 0;

      //TODO make sure that correct amount of locations are used,
      //it depends on the value size: 1 location <= 4 floats
      for (auto& output : outputs) {
         auto vkt = Node::DecayToGLSLType(output.GetType());
         if (not vkt) {
            Logger::Error("Unsupported base for shader output ",
               output.GetTag(), ": ", vkt, " (decayed from ",
               output.GetType(), ")"
            );
            LANGULUS_THROW(Material,
               "Unsupported base for shader output");
         }

         // Format the output                                           
         //    @param {0} - attribute location index                    
         //    @param {1} - type of the vertex attribute                
         //    @param {2} - name of the vertex attribute                
         constexpr auto layout = R"shader(
            layout(location = {0})
            out {1} {2};
         )shader";

         const GLSL type {vkt};
         const GLSL name {GenerateOutputName(rate, output)};
         const auto definition = Text::TemplateRt(layout, location, type, name);

         // Add output to code                                          
         Commit(rate, ShaderToken::Output, definition);
         ++location;
      }
   }
}

/// Initialize the material by using a shadertoy snippet                      
///   @param code - the code to port                                          
void Material::InitializeFromShadertoy(const GLSL& code) {
   Logger::Verbose("Code seems to be from shadertoy.com - porting:");
   Logger::Verbose(code.Pretty());

   // Add the code snippet to the pixel shader                          
   Commit(Rate::Pixel, ShaderToken::Functions, code);

   // Shadertoy has the vertical flipped, so use this iFragment macro   
   Commit(Rate::Pixel, ShaderToken::Defines,
      "#define iFragment vec2(gl_FragCoord.x, iResolution.y - gl_FragCoord.y)");

   // Maps a code token to an input tag by using a macro              
   auto integrate = [&](Tag const& tag, Token const& keyword) {
      if (not code.FindKeyword(keyword))
         return;

      const auto input = AddInput(Rate::Pixel, tag, false);
      constexpr auto layout = R"shader(
         #define {1} {2}
      )shader";

      Commit(Rate::Pixel, ShaderToken::Defines,
         Text::TemplateRt(layout, keyword, input));
   };

   // Satisfy tags                                                    
   integrate(Tags::Time  {}, "iTime");
   integrate(Tags::Size  {}, "iResolution");
   integrate(Tags::Image {}, "iChannel0");
   integrate(Tags::View  {}, "iView");

   /*TODO
   for snippets that are not from shadertoy, search tag symbols
   integrate(Tag::From<Tags::ViewProjectTransformInverted>());
   integrate(Tag::From<Tags::ViewProjectTransform>());
   integrate(Tag::From<Tags::ViewTransformInverted>());
   integrate(Tag::From<Tags::ViewTransform>());
   integrate(Tag::From<Tags::Resolution>());
   integrate(Tag::From<Tags::Time>());
   integrate(Tag::From<Tags::FOV>());*/

   // Wrap the shadertoy's mainImage function in our own ShadertoyMain  
   Commit(Rate::Pixel, ShaderToken::Functions, R"shader(
      vec4 ShadertoyMain() {
         vec4 output;
         mainImage(output, iFragment);
         return output;
      }
   )shader");

   // Add output color per pixel                                        
   auto output = AddOutput(Rate::Pixel, Tags::Color::OfType<Vec4>(), false);
   Commit(Rate::Pixel, ShaderToken::Colorize, output + " = ShadertoyMain();");
}
