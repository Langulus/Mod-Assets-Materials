#include "MContent.hpp"

/// Material generator construction															
///	@param producer - the producer module												
CGeneratorMaterial::CGeneratorMaterial(MContent* producer)
	: AMaterial {MetaData::Of<CGeneratorMaterial>()}
	, TProducedFrom {producer}
	, mRoot {this} {
	mConstruct = { ClassMeta() };

	// Allocate the shader stages														
	TAny<GLSL> stages;
	stages.Allocate(ShaderStage::Counter, true);
	SaveData<Traits::Code>(pcMove(stages));

	// Allocate the uniform rates + output rates									
	TAny<TAny<Trait>> inputsAndOutputs;
	inputsAndOutputs.Allocate(RRate::InputCount + ShaderStage::Counter, true);
	SaveData<Traits::Trait>(pcMove(inputsAndOutputs));

	ClassValidate();
}

/// Material destruction																		
CGeneratorMaterial::~CGeneratorMaterial() {
	Uninitialize();
}

/// Initialize the material using the descriptor										
///	@return true on success																	
void CGeneratorMaterial::Generate() {
	if (mGenerated)
		return;

	if (!mCode.IsEmpty()) {
		// Compile the code for the material										
		const auto parsed { mCode.Parse() };
		if (!mCompiled.Push(parsed))
			throw Except::Content(pcLogSelfError << "Couldn't parse material code");

		// Execute the compiled code in this material's context				
		// In this context, the period is symbolic, and for it we use the	
		// maximal refresh rate dedicated to shaders (i.e. per pixel)		
		PC_VERBOSE_MATERIAL_TAB("Now building nodes..");
		auto thisBlock { GetBlock() };
		mCompiled.Execute(thisBlock, 0, RRate::Counter);
	}

	// Auto-complete the rest of the material										
	AutoComplete();

	// Success																				
	PC_VERBOSE_MATERIAL(ccGreen << "Generated " << ClassToken());
	mGenerated = true;
} 

/// Create/destroy stuff in material context												
///	@param verb - the creation verb														
void CGeneratorMaterial::Create(Verb& verb) {
	const auto verbRate = MaterialNode::Rate(verb, mDefaultRate);
	pcptr inputIdx = 0, outputIdx = 0;
	verb.GetArgument().ForEachDeep([&](const Block& group) {
		group.ForEach([&](const RRate& rate) {
			// Set default rate															
			PC_VERBOSE_MATERIAL("Configuring default rate: " << rate);
			mDefaultRate = rate;
			SetTraitValue<Traits::Frequency>(mDefaultRate);
		});

		group.ForEach([&](const GASM& code) {
			// Add a GASM code snippet													
			PC_VERBOSE_MATERIAL("Appending GASM code: " << code);
			if (!mCode.IsEmpty())
				mCode += ", ";
			mCode += code;
			SetTraitValue<Traits::Code>(mCode);
		});

		group.ForEach([&](const GLSL& code) {
			// Add a GLSL code snippet													
			PC_VERBOSE_MATERIAL("Configuring via GLSL code: " << code);
			if (code.FindWild("void*mainImage*(*out*vec4*fragColor*,*in*vec2*fragCoord*)*{"))
				InitializeFromShadertoy(code);
			else throw Except::Content(pcLogSelfError 
				<< "Unsupported GLSL snippet for material: " << code.Pretty());
		});

		group.ForEach([&](const Trait& trait) {
			// Add an input/output														
			if (trait.GetTraitID() == Traits::Input::ID) {
				PC_VERBOSE_MATERIAL("Creating an explicit input/uniform via trait: " << trait);
				auto newNode = Ptr<MaterialNodeValue>::New(
					MaterialNodeValue::Input(this, trait.As<Trait>(), verbRate));
				mRoot.AddChild(newNode);
				verb << newNode;
				//SetTraitValue<Traits::Input>(trait.As<Trait>(), inputIdx);
				++inputIdx;
			}
			else if (trait.GetTraitID() == Traits::Output::ID) {
				PC_VERBOSE_MATERIAL("Creating an explicit output via trait: " << trait);
				auto newNode = Ptr<MaterialNodeValue>::New(
					MaterialNodeValue::Output(this, trait.As<Trait>(), verbRate));
				mRoot.AddChild(newNode);
				verb << newNode;
				//SetTraitValue<Traits::Output>(trait.As<Trait>(), outputIdx);
				++outputIdx;
			}
			else TODO();
		});
	});

	verb.Done();
}

/// Interface material inputs/outputs at a global level								
///	@param verb - the selection verb														
void CGeneratorMaterial::Select(Verb& verb) {
	mRoot.Select(verb);
}

/// Projection mapping																			
///	@param verb - projection verb															
void CGeneratorMaterial::Project(Verb& verb) {
	auto newNode = Ptr<MaterialNodeCamera>::New(&mRoot, verb);
	mRoot.AddChild(newNode);
	verb << newNode;
}

/// Raymarching using signed distance field functions									
///	@param verb - projection verb															
void CGeneratorMaterial::Raymarch(Verb& verb) {
	auto newNode = Ptr<MaterialNodeRaymarch>::New(&mRoot, verb);
	mRoot.AddChild(newNode);
	verb << newNode;
}

/// Rasterize scene to screen																	
///	@param verb - rasterization verb														
void CGeneratorMaterial::Rasterize(Verb& verb) {
	auto newNode = Ptr<MaterialNodeRasterize>::New(&mRoot, verb);
	mRoot.AddChild(newNode);
	verb << newNode;
}

/// Raycast a scene																				
///	@param verb - raycast verb																
void CGeneratorMaterial::Raycast(Verb& verb) {
	auto newNode = Ptr<MaterialNodeRaycast>::New(&mRoot, verb);
	mRoot.AddChild(newNode);
	verb << newNode;
}

/// Raytrace a scene																				
///	@param verb - raytrace verb															
void CGeneratorMaterial::Raytrace(Verb& verb) {
	auto newNode = Ptr<MaterialNodeRaytrace>::New(&mRoot, verb);
	mRoot.AddChild(newNode);
	verb << newNode;
}

/// Texturize material, applying solid colors, color maps, normal	maps, etc.	
///	@param verb - the texturization verb												
void CGeneratorMaterial::Texturize(Verb& verb) {
	auto oldNode = mRoot.FindChild<MaterialNodeTexture>();
	if (oldNode) {
		oldNode->Texturize(verb);
		return;
	}

	auto newNode = Ptr<MaterialNodeTexture>::New(&mRoot, verb);
	mRoot.AddChild(newNode);
	newNode->Texturize(verb);
	verb << newNode;
}

/// Illumination/darkening based on dot products										
///	@param verb - projection verb															
void CGeneratorMaterial::Illuminate(Verb& verb) {
	auto newNode = Ptr<MaterialNodeLight>::New(&mRoot, verb);
	mRoot.AddChild(newNode);
	verb << newNode;
}

/// Dump hierarchy to log																		
void CGeneratorMaterial::Dump() const {
	mRoot.Dump();
}

/// Autocomplete outputs																		
void CGeneratorMaterial::AutoComplete() {
	PC_VERBOSE_MATERIAL("Now auto-completing...");

	// Start generating																	
	mRoot.Generate();

	// If a vertex shader is missing, add a default one						
	GLSL& vs = GetStage(ShaderStage::Vertex);
	if (vs.IsEmpty()) {
		// Default vertex stage - a rectangle filling the screen				
		Commit(ShaderStage::Vertex, ShaderToken::Transform,
			"const vec2 outUV = vec2(gl_VertexIndex & 2, (gl_VertexIndex << 1) & 2);\n"
			"gl_Position = vec4(outUV * 2.0f - 1.0f, 0.0f, 1.0f);\n"_glsl
		);
	}

	// Generate inputs and outputs where needed									
	GenerateInputs();
	GenerateOutputs();
	GenerateUniforms();

	// Finish all the stages															
	for (pcptr i = 0; i < GetStages().GetCount(); ++i) {
		auto& stage = GetStage(ShaderStage::Enum(i));
		if (stage.IsEmpty())
			continue;

		// Write shader version to all relevant codes							
		stage.SetVersion("450");

		PC_VERBOSE_MATERIAL("Stage (" << ShaderStage::Names[i] << "):\n");
		PC_VERBOSE_MATERIAL(stage.Pretty());
	}
}

/// Generate input name																			
///	@param rate - the rate at which the input is declared							
///	@param trait - the trait tag for the input										
///	@return the variable name to access the input									
GLSL CGeneratorMaterial::GenerateInputName(RRate rate, const Trait& trait) const {
	if (trait.TraitIs<Traits::Texture>()) {
		// Samplers are handled differently											
		return GLSL(TraitID::Prefix) + trait.GetTraitMeta()->GetToken() + mConsumedSamplers;
	}
	else if (!rate.IsUniform()) {
		// Name is for a vertex attribute or varying								
		return "in"_glsl + trait.GetTraitMeta()->GetToken();
	}

	// Uniform name inside a uniform buffer										
	const GLSL UBOname = RRate::Names[static_cast<pcptr>(rate)];
	const GLSL name = GLSL(TraitID::Prefix) + trait.GetTraitMeta()->GetToken();
	return UBOname + "." + name;
}

/// Generate output name																		
///	@param rate - the rate at which the output is declared						
///	@param trait - the trait tag for the output										
///	@return the variable name to access the output									
GLSL CGeneratorMaterial::GenerateOutputName(RRate rate, const Trait& trait) const {
	if (!rate.IsShaderStage())
		throw Except::Content(pcLogSelfError 
			<< "Can't have an output outside a shader stage rate: "
			<< trait << " @ " << rate);
	return "out"_glsl + trait.GetTraitMeta()->GetToken();
}

/// Generate uniform buffer descriptions for a specific stage						
///	@param stage - the stage to populate												
void CGeneratorMaterial::GenerateUniforms() {
	const auto uniforms = GetData<Traits::Trait>();

	// Scan all uniform rates															
	for (pcptr i = 0; i < RRate::UniformCount; ++i) {
		auto& traits = uniforms->As<TAny<Trait>>(i);
		if (traits.IsEmpty())
			continue;

		// Define the rate-dedicated uniform buffer								
		const auto rate = RRate(i + RRate::UniformBegin);
		const GLSL UBOname = RRate::Names[pcptr(rate)];
		GLSL ubo;
		if (rate.IsStaticUniform()) {
			ubo =
				"layout(set = 0, binding = "_glsl +
				rate.GetStaticUniformIndex() +
				") uniform UniformBuffer" + UBOname + " {\n";
		}
		else if (rate.IsDynamicUniform()) {
			ubo =
				"layout(set = 1, binding = "_glsl +
				rate.GetDynamicUniformIndex() +
				") uniform UniformBuffer" + UBOname + " {\n";
		}
		else throw Except::Content(pcLogSelfError
			<< "Uniform rate not static or dynamic: " << rate);

		// Declare all relevant traits inside										
		TAny<GLSL> names;
		for (auto& trait : traits) {
			auto tmeta = trait.GetTraitMeta();
			if (tmeta->Is<Traits::Texture>()) {
				// Skip textures - they are taken care of later					
				continue;
			}

			auto dmeta = trait.GetMeta();
			if (!dmeta) {
				throw Except::Content(pcLogSelfError
					<< "Bad input type for " << trait);
			}

			const auto name = GLSL(TraitID::Prefix) + tmeta->GetToken();
			const auto definition = "   "_glsl + GLSL::Type(dmeta) + " " 
				+ name + (&traits.Last() == &trait ? ";" : ";\n");

			ubo += definition;
			names << name;
			PC_VERBOSE_MATERIAL("Added uniform: " << UBOname << "." << definition);
		}

		if (names.IsEmpty())
			continue;

		// Close the ubo scope and write to code									
		ubo += "\n} " + UBOname + ";\n\n";

		// Add the uniform block to each stage it is used in					
		for (auto& code : GetStages()) {
			for (const auto& name : names) {
				if (code.Find(name)) {
					// Add ubo only in stages that use it							
					code.Select(ShaderToken::Uniform) >> ubo;
					break;
				}
			}
		}
	}

	// Do another scan for the textures												
	pcptr textureNumber = 0;
	const auto perRenderable = RRate(RRate::PerRenderable).GetInputIndex();
	auto& traits = uniforms->As<TAny<Trait>>(perRenderable);
	for (auto& trait : traits) {
		auto tmeta = trait.GetTraitMeta();
		if (!tmeta->Is<Traits::Texture>())
			continue;

		const auto uniform = "layout(set = 2, binding = "_glsl + textureNumber + ") uniform ";
		const auto name = GLSL(TraitID::Prefix) + Traits::Texture::Reflect()->GetToken();
		//TODO check texture format
		//TODO we don't support different kinds of samplers yet :(
		//auto dmeta = trait.GetMeta();
		const auto definition = uniform + "sampler2D " + name + textureNumber + ";\n";
		PC_VERBOSE_MATERIAL("Added sampler(s): " << definition);

		// Add the uniform block to each stage it is used in						
		for (auto& code : GetStages()) {
			if (code.Find(name)) {
				// Add samplers only in stages that use them							
				code.Select(ShaderToken::Uniform) >> definition;
				break;
			}
		}

		++textureNumber;
	}
}

/// Generate vertex attributes (aka vertex shader inputs)							
void CGeneratorMaterial::GenerateInputs() {
	const auto uniforms = GetData<Traits::Trait>();
	for (pcptr i = 0; i < ShaderStage::Counter; ++i) {
		const auto stage = ShaderStage::Enum(i);
		const auto rate = RRate::StagesBegin + i;
		const auto& inputs = uniforms->Get<TAny<Trait>>(rate - RRate::UniformBegin);
		//TODO make sure that correct amount of locations are used,			
		//it depends on the value size: 1 location <= 4 floats				
		pcptr uidx = 0;
		for (auto& input : inputs) {
			auto vkt = MaterialNode::DecayToGLSLType(input.GetMeta());
			if (!vkt) {
				throw Except::Content(pcLogSelfError
					<< "Unsupported base for shader attribute "
					<< input.GetTraitMeta()->GetToken()
					<< ": " << vkt << " (decayed from "
					<< input.GetToken() << ")"
				);
			}

			// Format the input															
			const auto prefix = "layout(location = "_glsl + uidx + ") in ";
			const auto final = prefix + GLSL::Type(vkt) + " " + GenerateInputName(rate, input) + ";\n";

			// Add input to code															
			PC_VERBOSE_MATERIAL("Added input: " << final);
			Commit(stage, ShaderToken::Input, final);
			++uidx;
		}
	}
}

/// Generate shader outputs																	
void CGeneratorMaterial::GenerateOutputs() {
	bool noExplicitOutputs = true;
	const auto traits = GetData<Traits::Trait>();
	for (pcptr i = 0; i < ShaderStage::Counter; ++i) {
		const auto stage = ShaderStage::Enum(i);
		const auto rate = RRate::StagesBegin + i;
		const auto& outputs = traits->Get<TAny<Trait>>(RRate::InputCount + i);
		//TODO make sure that correct amount of locations are used,			
		//it depends on the value size: 1 location <= 4 floats				
		pcptr uidx = 0;
		for (auto& output : outputs) {
			auto vkt = MaterialNode::DecayToGLSLType(output.GetMeta());
			if (!vkt) {
				throw Except::Content(pcLogSelfError
					<< "Unsupported base for shader output "
					<< output.GetTraitMeta()->GetToken()
					<< ": " << vkt << " (decayed from "
					<< output.GetToken() << ")"
				);
			}

			// Format the input															
			const auto prefix = "layout(location = "_glsl + uidx + ") out ";
			const auto name = GenerateOutputName(rate, output);
			const auto final = prefix + GLSL::Type(vkt) + " " + name + ";\n";

			// Add input to code															
			PC_VERBOSE_MATERIAL("Added output: " << final);
			noExplicitOutputs = false;
			Commit(stage, ShaderToken::Output, final);

			// Find a similar trait to output										
			auto test = GetValue(output.GetTraitMeta(), nullptr, rate);
			if (test.IsValid()) {
				Commit(stage, ShaderToken::Colorize,
					name + " = " + test.GetOutputSymbolAs(vkt, 0) + ";\n");
			}
			else {
				pcLogSelfWarning << "Can't find trait " << output << " in hierarchy for explicit output: " << final;
				pcLogSelfWarning << "Defaulting to solid red color";
				test = MaterialNodeValue::Local(&mRoot, Trait::From<Traits::Color>(Colors::Red));
				Commit(stage, ShaderToken::Colorize,
					name + " = " + test.GetOutputSymbolAs(vkt, 0) + ";\n");
			}
			++uidx;
		}
	}

	if (noExplicitOutputs) {
		// Automatically add output if nothing is available					
		// It relies on any color symbol available in the hierarchy			
		// Defaults to a vec4 color output											
		auto test = GetValue<Traits::Color>(RRate::PerPixel);
		auto trait = test.GetOutputTrait();
		const auto prefix = "layout(location = 0) out "_glsl;
		const auto name = GenerateOutputName(RRate::PerPixel, trait);
		const auto final = prefix + GLSL::Type<vec4f>() + " " + name + ";\n";
		Commit(ShaderStage::Pixel, ShaderToken::Output, final);

		if (test.IsValid()) {
			Commit(ShaderStage::Pixel, ShaderToken::Colorize,
				name + " = " + test.GetOutputSymbolAs(MetaData::Of<rgba>(), 0) + ";\n");
			PC_VERBOSE_MATERIAL("Implicit output " << name << " added as " << test);
		}
		else {
			pcLogSelfWarning << "Can't find color trait in hierarchy for implicit output: " << final;
			pcLogSelfWarning << "Defaulting to solid red color";
			test = MaterialNodeValue::Local(&mRoot, Trait::From<Traits::Color>(Colors::Red));
			Commit(ShaderStage::Pixel, ShaderToken::Colorize,
				name + " = " + test.GetOutputSymbolAs(MetaData::Of<rgba>(), 0) + ";\n");
		}
	}
}

/// Get a value from the node hierarchy													
///	@param tmeta - the trait to search for (use nullptr for any trait)		
///	@param dmeta - the data to search for (use nullptr for any data)			
///	@param rate - the rate of the data to search for								
///	@return a value interface																
MaterialNodeValue CGeneratorMaterial::GetValue(TMeta tmeta, DMeta dmeta, RRate rate) {
	MaterialNodeValue result(this);
	const auto prototype = Trait::FromMeta(tmeta, dmeta);
	RRate bestRate = RRate::PerNone;
	mRoot.ForEachChild([&](const MaterialNode& node) {
		PC_VERBOSE_MATERIAL("Searching for " << tmeta->GetToken() << " inside " << node);
		if (node.GetRate() > rate || !node.IsInput())
			return;

		for (auto& key : node.mOutputs.Keys()) {
			if (!prototype.IsSimilar(key) || node.GetRate() < result.GetRate())
				continue;

			PC_VERBOSE_MATERIAL(ccGreen << "Found " << key << " inside " << node);
			result.BindTo(key, &node);
		}
	});

	return result;
}


/// Initialize the material by using a shadertoy snippet								
///	@param code - the code to port														
void CGeneratorMaterial::InitializeFromShadertoy(const GLSL& code) {
	SetTraitValue<Traits::Code>(code, 1);
	pcLogSelfVerbose << "Code seems to be from shadertoy.com - porting:";
	pcLogSelfVerbose << code.Pretty();

	// Add the code snippet																
	Commit(ShaderStage::Pixel, ShaderToken::Functions, code);

	// Shadertoy has the vertical flipped, so use this iFragment macro	
	Commit(ShaderStage::Pixel, ShaderToken::Defines,
		"#define iFragment vec2(gl_FragCoord.x, iResolution.y - gl_FragCoord.y)\n");

	auto integrate = [&](const Trait& trait, const GLSL& wildcard) {
		if (!code.FindWild(wildcard))
			return;
		Commit(ShaderStage::Pixel, ShaderToken::Defines, 
			"#define " + wildcard.Strip('*') + " " + AddInput(RRate::PerAuto, trait, false) + "\n");
	};

	// Satisfy traits																		
	integrate(Trait::From<Traits::Time>(), "*iTime*");
	integrate(Trait::From<Traits::Resolution>(), "*iResolution*");
	integrate(Trait::From<Traits::Texture>(), "*iChannel0*");
	integrate(Trait::From<Traits::ViewTransform>(), "*iView*");
	/*TODO
	for snippets that are not from shadertoy, search trait symbols
	integrate(Trait::From<Traits::ViewProjectTransformInverted>());
	integrate(Trait::From<Traits::ViewProjectTransform>());
	integrate(Trait::From<Traits::ViewTransformInverted>());
	integrate(Trait::From<Traits::ViewTransform>());
	integrate(Trait::From<Traits::Resolution>());
	integrate(Trait::From<Traits::Time>());
	integrate(Trait::From<Traits::FOV>());*/

	// Add output color																	
	auto output = AddOutput(RRate::PerPixel, Trait::From<Traits::Color, vec4>(), false);
	Commit(ShaderStage::Pixel, ShaderToken::Colorize, "vec4 c;\nmainImage(c, iFragment);\n");

	auto color = Ptr<MaterialNodeValue>::New(
		MaterialNodeValue::Local(&mRoot, Trait::From<Traits::Color, vec4f>(), RRate::PerPixel, "c"));
	mRoot.AddChild(color);
}
