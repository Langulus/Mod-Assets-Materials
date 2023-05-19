#include "MContent.hpp"
#include "Shader/ShaderToy.h"

PC_MONOPOLIZE_MEMORY(0);

/// Content module construction																
///	@param system - the system that owns the module									
///	@param handle - the module handle													
MContent::MContent(CRuntime* system, PCLIB handle)
	: AModuleContent {MetaData::Of<MContent>(), system, handle}
	, mModels {this}
	, mSounds {this}
	, mTextures {this}
	, mMaterials {this}
	, mFonts {this} {
	pcLogSelfVerbose << "Initializing...";
	mModelsFolder = system->GetFolder("Content/Geometry");
	mSoundsFolder = system->GetFolder("Content/Sounds");
	mTexturesFolder = system->GetFolder("Content/Textures");
	mMaterialsFolder = system->GetFolder("Content/Materials");
	mFontsFolder = system->GetFolder("Content/Fonts");
	ClassValidate();
	pcLogSelfVerbose << "Initialized";
}

/// Content module update routine															
///	@param dt - time from last update													
void MContent::Update(PCTime) {

}

/// Create content generators																	
///	@param verb - the creation verb														
void MContent::Create(Verb& verb) {
	if (verb.GetArgument().IsEmpty())
		return;

	// Create content generators...													
	Any results;
	const auto create = [this, &results](const Construct& construct) {
		switch (construct.GetMeta()->GetSwitch()) {
		// ... for models																	
		case DataID::Switch<CGeometryBox>():
			Generate<CGeometryBox>(construct, results);
			break;
		case DataID::Switch<CGeometryCylinder>():
			Generate<CGeometryCylinder>(construct, results);
			break;
		case DataID::Switch<CGeometryFrustum>():
			Generate<CGeometryFrustum>(construct, results);
			break;
		case DataID::Switch<CGeometrySphere>():
			Generate<CGeometrySphere>(construct, results);
			break;
		case DataID::Switch<CGeometryGrid>():
			Generate<CGeometryGrid>(construct, results);
			break;
		case DataID::Switch<CGeometryLine>():
			Generate<CGeometryLine>(construct, results);
			break;
		case DataID::Switch<CGeometryRectangle>():
			Generate<CGeometryRectangle>(construct, results);
			break;
		case DataID::Switch<CGeometryText>():
			Generate<CGeometryText>(construct, results);
			break;
		case DataID::Switch<CGeometryTree>():
			Generate<CGeometryTree>(construct, results);
			break;
		case DataID::Switch<CGeometryTriangle>():
			Generate<CGeometryTriangle>(construct, results);
			break;
		case DataID::Switch<CGeometryZode>():
			Generate<CGeometryZode>(construct, results);
			break;

		// ... for textures																
		case DataID::Switch<ATexture>():
		case DataID::Switch<CGeneratorTexture>():
			Generate<CGeneratorTexture>(construct, results);
			break;

		// ... for fonts																	
		case DataID::Switch<AFont>():
		case DataID::Switch<CGeneratorFont>():
			Generate<CGeneratorFont>(construct, results);
			break;

		// ... for materials																
		case DataID::Switch<AMaterial>():
		case DataID::Switch<CGeneratorMaterial>():
			Generate<CGeneratorMaterial>(construct, results);
			break;
		}

		return true;
	};

	// Scan the request																	
	verb.GetArgument().ForEachDeep([&create](const Block& group) {
		group.ForEach([&create](const Construct& construct) {
			create(construct);
		});
		group.ForEach([&create](const DataID& type) {
			create(Construct(type));
		});
		group.ForEach([&create](const MetaData* type) {
			create(Construct(type));
		});
	});

	verb << pcMove(results);
}

/// Get a font by name, or load it if not available									
///	@param name - the font name															
///	@return the font interface if found or loaded									
CGeneratorFont* MContent::GetFont(const Text& name) {
	// Search by name only																
	for (auto& font : mFonts) {
		if (font.IsClassValid() && font.GetFontName() == name)
			return &font;
	}

	// Construct a new font with the given name									
	auto request = Construct::From<AFont>(name);
	Any result;
	if (Generate<CGeneratorFont>(request, result))
		return result.As<CGeneratorFont*>();
	return nullptr;
}

/// Get a texture by name, or load it if not available								
///	@param name - the texture name														
///	@return the texture interface if found or loaded								
CGeneratorTexture* MContent::GetTexture(const Text& name) {
	auto request = Construct::From<ATexture>(name);
	Any result;
	if (Generate<CGeneratorTexture>(request, result))
		return result.As<CGeneratorTexture*>();
	return nullptr;
}

/// Get a material by name, or load it if not available								
///	@param name - the material name														
///	@return the material interface if found or loaded								
CGeneratorMaterial* MContent::GetMaterial(const Text& name) {
	auto request = Construct::From<AMaterial>(name);
	Any result;
	if (Generate<CGeneratorMaterial>(request, result))
		return result.As<CGeneratorMaterial*>();
	return nullptr;
}

/// Get a sound by name, or load it if not available									
///	@param name - the sound name															
///	@return the sound interface if found or loaded									
CGeneratorSound* MContent::GetSound(const Text& name) {
	auto request = Construct::From<ASound>(name);
	Any result;
	if (Generate<CGeneratorSound>(request, result))
		return result.As<CGeneratorSound*>();
	return nullptr;
}

/// Get a model by name, or load it if not available									
///	@param name - the model name															
///	@return the model interface if found or loaded									
/*CGeneratorGeometry* MContent::GetModel(const Text& name) {
	auto request = Construct::From<AGeometry>(name);
	Any result;
	if (Generate<CGeneratorGeometry>(request, result))
		return result.As<CGeneratorGeometry*>();
	return nullptr;
}*/
