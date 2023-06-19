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
