///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright(C) 2016 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Material.hpp"
#include "MaterialLibrary.hpp"
#include <Anyness/Edit.hpp>


/// Material construction                                                     
///   @param producer - the producer                                          
///   @param descriptor - instructions for configuring the material           
Material::Material(A::AssetModule* producer, const Descriptor& descriptor)
   : A::Material {MetaOf<Material>(), producer, descriptor}
   , mRoot {this, descriptor} {
   Logger::Verbose(Self(), "Initializing...");
   // Extract default rate if any                                       
   if (!mDescriptor.ExtractTrait<Traits::Rate>(mDefaultRate))
      mDescriptor.ExtractData(mDefaultRate);
   Logger::Verbose(Self(), "Initialized");
}

/// Create nodes inside the material                                          
///   @param verb - creation verb                                             
void Material::Create(Verb& verb) {
   mRoot.Create(verb);
}

/// Get material adapter for lower or higher level of detail                  
///   @param lod - the level-of-detail state                                  
///   @return a pointer to the material generator                             
const A::Material* Material::GetLOD(const LOD& lod) const {
   TODO();
}

/// Get default material rate                                                 
///   @return the rate                                                        
const Rate& Material::GetDefaultRate() const noexcept {
   return mDefaultRate;
}

/// Commit a code snippet to a specific stage and place                       
///	@param stage - the shader stage to commit to                            
///   @param place - the shader token to commit changes at                    
///   @param addition - the code to commit                                    
void Material::Commit(Rate rate, const Token& place, const Token& addition) {
   const auto stage = rate.GetStageIndex();
   auto& code = GetStage(stage);
   if (!code) {
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
GLSL& Material::GetStage(Offset stage) {
   LANGULUS_ASSUME(DevAssumes, stage < ShaderStage::Counter,
      "Bad stage offset");
   return const_cast<GLSL&>(GetStages()[stage]);
}

/// Get a GLSL stage (const)                                                  
///   @param stage - the stage index                                          
///   @return the code associated with the stage                              
const GLSL& Material::GetStage(Offset stage) const {
   return const_cast<Material*>(this)->GetStage(stage);
}

/// Get the list of GLSL stages                                               
///   @return the list                                                        
TAny<GLSL>& Material::GetStages() {
   auto stages = GetDataList<Traits::Data>();
   LANGULUS_ASSUME(DevAssumes, stages,
      "No data inside material");
   LANGULUS_ASSUME(DevAssumes, stages->GetCount() == ShaderStage::Counter,
      "Bad material data count");
   LANGULUS_ASSUME(DevAssumes, stages->template IsExact<GLSL>(),
      "Material data type mismatch");

   return const_cast<TAny<GLSL>&>(
      *reinterpret_cast<const TAny<GLSL>*>(stages)
   );
}

/// Get the list of GLSL stages (const)                                       
///   @return the list                                                        
const TAny<GLSL>& Material::GetStages() const {
   return const_cast<Material*>(this)->GetStages();
}

/// Add an external input trait                                               
///   @param rate - the rate at which the input will be refreshed             
///   @param traitOriginal - the input to add                                 
///   @param allowDuplicates - whether multiple such traits are allowed       
///   @return the generated symbol name                                       
GLSL Material::AddInput(Rate rate, const Trait& traitOriginal, bool allowDuplicates) {
   // An input must always be declared with a rate, and that rate       
   // must always be smaller or equal to the node's rate                
   // For example, you might want Time input PerPixel, but the          
   // actual uniform will be updated PerTick. So this is                
   // where we step in to override any wrongly provided rate            
   if (rate == Rate::Auto)
      rate = Node::GetDefaultTrait(traitOriginal.GetTrait()).mRate;

   // Find any matching available inputs                                
   auto& inputs = const_cast<TraitList&>(GetInputs(rate));
   if (!allowDuplicates) {
      auto found = inputs.Find(traitOriginal);
      if (!found.IsSpecial())
         return GenerateInputName(rate, inputs[found]);
   }
   
   // Add the new input                                                 
   auto trait = traitOriginal;
   if (trait.IsUntyped())
      trait.SetType(Node::GetDefaultTrait(trait.GetTrait()).mType);

   inputs << trait;
   const auto symbol = GenerateInputName(rate, trait);
   if (trait.TraitIs<Traits::Texture>())
      ++mConsumedSamplers;

   VERBOSE_NODE("Added input ", Logger::Cyan, trait, " as `", symbol, "` @ ", rate);
   return symbol;
}

/// Add an output to the material                                             
///   @param rate - the rate at which the output is refreshed                 
///   @param traitOriginal - the output to add                                
///   @param allowDuplicates - whether multiple such traits are allowed       
///   @return the generated symbol name                                       
GLSL Material::AddOutput(Rate rate, const Trait& traitOriginal, bool allowDuplicates) {
   LANGULUS_ASSERT(rate.IsShaderStage(), Material,
      "Can't add material outputs to rates, "
      "that don't correspond to shader stages");

   auto& outputs = const_cast<TraitList&>(GetOutputs(rate));
   if (!allowDuplicates) {
      auto found = outputs.Find(traitOriginal);
      if (!found.IsSpecial())
         return GenerateOutputName(rate, outputs[found]);
   }

   // Add the new output                                                
   auto trait = traitOriginal;
   if (trait.IsUntyped())
      trait.SetType(Node::GetDefaultTrait(trait.GetTrait()).mType);

   outputs << trait;
   const auto symbol = GenerateOutputName(rate, trait);
   VERBOSE_NODE("Added output ", Logger::Cyan, trait, " as `", symbol, "` @ ", rate);
   return symbol;
}

/// Adds a code snippet                                                       
///   @param rate - the shader stage to place code at                         
///   @param name - the name of the definition (to check for duplicated)      
///   @param code - the code to insert                                        
void Material::AddDefine(Rate rate, const Token& name, const GLSL& code) {
   const auto stageIndex = rate.GetStageIndex();
   mDefinitions[stageIndex][name] << code;
}

/// Generate input name                                                       
///   @param rate - the rate at which the input is declared                   
///   @param trait - the trait tag for the input                              
///   @return the variable name to access the input                           
GLSL Material::GenerateInputName(Rate rate, const Trait& trait) const {
   if (trait.TraitIs<Traits::Texture>()) {
      // Samplers are handled differently                               
      return GLSL {trait.GetTrait()} + mConsumedSamplers;
   }
   else if (!rate.IsUniform()) {
      // Name is for a vertex attribute or varying                      
      return "in"_glsl + trait.GetToken();
   }

   // Uniform name inside a uniform buffer                              
   return Text::TemplateRt("{}.{}", GLSL {rate}, GLSL {trait.GetTrait()});
}

/// Generate output name                                                      
///   @param rate - the rate at which the output is declared                  
///   @param trait - the trait tag for the output                             
///   @return the variable name to access the output                          
GLSL Material::GenerateOutputName(Rate rate, const Trait& trait) const {
   LANGULUS_ASSERT(rate.IsShaderStage(), Material,
      "Can't have an output outside a shader stage rate");
   return "out"_glsl + trait.GetToken();
}

/// Generate uniform buffer descriptions for all shader stages                
void Material::GenerateUniforms() {
   // Scan all uniform rates:                                           
   // Tick, Pass, Camera, Level, Renderable, Instance                   
   for (Offset i = 0; i < Rate::UniformCount; ++i) {
      const Rate rate {i + Rate::UniformBegin};
      auto& traits = GetInputs(i);
      if (!traits)
         continue;

      // Define the rate-dedicated uniform buffer, if there's at least  
      // one trait available                                            
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
      TAny<GLSL> names;
      for (auto& trait : traits) {
         // Skip textures for now                                       
         if (trait.TraitIs<Traits::Texture>())
            continue;

         LANGULUS_ASSERT(trait.IsTyped(),  Material,
            "Uniform is not typed");
         LANGULUS_ASSERT(trait.GetTrait(), Material,
            "Uniform has undefined trait");

         const GLSL type {trait.GetType()};
         const GLSL name {trait.GetTrait()};
         body += Text::TemplateRt("{} {};", type, name);
         if (&trait != &traits.Last())
            body += '\n';
         names << name;
      }

      if (!names)
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
      else LANGULUS_THROW(Material, "Uniform rate neither static, nor dynamic");

      // Add the uniform buffer to each stage it is used in             
      for (auto& code : GetStages()) {
         for (const auto& name : names) {
            if (!code.Find(name))
               continue;

            Edit(code).Select(ShaderToken::Uniform) << ubo;
            break;
         }
      }
   }

   // Do another scan for the textures                                  
   // Textures are always updated per renderable for now                
   // Samplers always use layout set #2 for now                         
   Offset textureNumber = 0;
   auto& traits = GetInputs(PerRenderable);
   for (auto& trait : traits) {
      // Skip anything BUT textures                                     
      if (!trait.TraitIs<Traits::Texture>())
         continue;

      // Define each sampler using this layout                          
      //    @param {0} - binding index, each unique texture view should 
      //                 get a unique index                             
      //    @param {1} - sampler type, deduced from the trait data type 
      //    @param {2} - the trait name, extracted from trait id        
      constexpr auto layout = R"shader(
         layout(set = 2, binding = {0})
         uniform {1} {2}{0};
      )shader";

      const GLSL type {trait.GetType()};
      const GLSL name {trait.GetTrait()};
      const auto ubo = Text::TemplateRt(layout, textureNumber, type, name);

      // Add texture to each stage it is used in                        
      for (auto& code : GetStages()) {
         if (!code.Find(name + textureNumber))
            continue;

         Edit(code).Select(ShaderToken::Uniform) << ubo;
      }

      ++textureNumber;
   }
}

/// Generate vertex attributes (aka vertex shader inputs)                     
void Material::GenerateInputs() {
   for (Offset i = 0; i < ShaderStage::Counter; ++i) {
      const Rate rate {Rate::StagesBegin + i};
      const auto& inputs = GetInputs(rate);
      Offset location = 0;

      //TODO make sure that correct amount of locations are used,
      //it depends on the value size: 1 location <= 4 floats
      for (auto& input : inputs) {
         auto vkt = Node::DecayToGLSLType(input.GetType());
         if (!vkt) {
            Logger::Error("Unsupported base for shader attribute ", 
               input.GetTrait(), ": ", vkt, " (decayed from ", 
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
   for (Offset i = 0; i < ShaderStage::Counter; ++i) {
      const Rate rate {Rate::StagesBegin + i};
      const auto& outputs = GetOutputs(rate);
      Offset location = 0;

      //TODO make sure that correct amount of locations are used,
      //it depends on the value size: 1 location <= 4 floats
      for (auto& output : outputs) {
         auto vkt = Node::DecayToGLSLType(output.GetType());
         if (!vkt) {
            Logger::Error("Unsupported base for shader output ",
               output.GetTrait(), ": ", vkt, " (decayed from ",
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
   Commit(PerPixel, ShaderToken::Functions, code);

   // Shadertoy has the vertical flipped, so use this iFragment macro   
   Commit(PerPixel, ShaderToken::Defines,
      "#define iFragment vec2(gl_FragCoord.x, iResolution.y - gl_FragCoord.y)");

   // Maps a code token to an input trait by using a macro              
   auto integrate = [&](const Trait& trait, const Token& keyword) {
      if (!code.FindKeyword(keyword))
         return;

      const auto input = AddInput(PerPixel, trait, false);
      constexpr auto layout = R"shader(
         #define {1} {2}
      )shader";

      Commit(PerPixel, ShaderToken::Defines,
         Text::TemplateRt(layout, keyword, input));
   };

   // Satisfy traits                                                    
   integrate(Traits::Time {}, "iTime");
   integrate(Traits::Size {}, "iResolution");
   integrate(Traits::Texture {}, "iChannel0");
   integrate(Traits::View {}, "iView");

   /*TODO
   for snippets that are not from shadertoy, search trait symbols
   integrate(Trait::From<Traits::ViewProjectTransformInverted>());
   integrate(Trait::From<Traits::ViewProjectTransform>());
   integrate(Trait::From<Traits::ViewTransformInverted>());
   integrate(Trait::From<Traits::ViewTransform>());
   integrate(Trait::From<Traits::Resolution>());
   integrate(Trait::From<Traits::Time>());
   integrate(Trait::From<Traits::FOV>());*/

   // Wrap the shadertoy's mainImage function in our own ShadertoyMain  
   Commit(PerPixel, ShaderToken::Functions, R"shader(
      vec4 ShadertoyMain() {
         vec4 output;
         mainImage(output, iFragment);
         return output;
      }
   )shader");

   // Add output color per pixel                                        
   auto output = AddOutput(PerPixel, Traits::Color::OfType<Vec4>(), false);
   Commit(PerPixel, ShaderToken::Colorize, output + " = ShadertoyMain();");
}
