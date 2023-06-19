#pragma once
#include "Material/MaterialNode.hpp"
#include "Material/MaterialNodeRoot.hpp"
#include "Material/MaterialNodeValue.hpp"
#include "Material/MaterialNodeScene.hpp"
#include "Material/MaterialNodeTransform.hpp"
#include "Material/MaterialNodeTexture.hpp"
#include "Material/MaterialNodeCamera.hpp"
#include "Material/MaterialNodeLight.hpp"
#include "Material/MaterialNodeRasterize.hpp"
#include "Material/MaterialNodeRaycast.hpp"
#include "Material/MaterialNodeRaymarch.hpp"
#include "Material/MaterialNodeRaytrace.hpp"
#include "Material/MaterialNodeFBM.hpp"

#define PC_VERBOSE_MATERIAL(a) pcLogSelfVerbose << a
#define PC_VERBOSE_MATERIAL_TAB(a) ScopedTab tab; pcLogSelfVerbose << a << tab


///																									
///	MATERIAL GENERATOR																		
///																									
class CGeneratorMaterial : public AMaterial, public TProducedFrom<MContent> {

public:
	REFLECT(CGeneratorMaterial);
	CGeneratorMaterial(MContent*);
	CGeneratorMaterial(CGeneratorMaterial&&) noexcept = default;
	CGeneratorMaterial& operator = (CGeneratorMaterial&&) noexcept = default;
	~CGeneratorMaterial();

public:
	PC_VERB(Create);
	PC_VERB(Select);
	PC_VERB(Project);
	PC_VERB(Raymarch);
	PC_VERB(Rasterize);
	PC_VERB(Raycast);
	PC_VERB(Raytrace);
	PC_VERB(Texturize);
	PC_VERB(Illuminate);

	void Generate() override;

	void Dump() const;

	GLSL AddInput(RRate, const Trait&, bool allowDuplicates);
	GLSL AddOutput(RRate, const Trait&, bool allowDuplicates);

	NOD() const GLSL& GetStage(ShaderStage::Enum) const;
	NOD() GLSL& GetStage(ShaderStage::Enum);
	NOD() TAny<GLSL>& GetStages();

	NOD() MaterialNodeValue GetValue(TMeta, DMeta, RRate);

	template<RTTI::ReflectedTrait TRAIT, RTTI::ReflectedData DATA = void>
	NOD() MaterialNodeValue GetValue(RRate rate) {
		return GetValue(TRAIT::Reflect(), DataID::Reflect<DATA>(), rate);
	}

	void Commit(ShaderStage::Enum, ShaderToken::Enum, const GLSL&);
	void InitializeFromShadertoy(const GLSL&);


private:
	void AutoComplete();
};