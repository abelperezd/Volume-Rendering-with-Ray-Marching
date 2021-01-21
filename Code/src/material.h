#ifndef MATERIAL_H
#define MATERIAL_H

#include "framework.h"
#include "shader.h"
#include "camera.h"
#include "mesh.h"

#include "volume.h"
#include "light.h"

class Material {
public:

	Shader* shader = NULL;
	Texture* texture = NULL;
	vec4 color;

	virtual void setUniforms(Camera* camera, Matrix44 model) = 0;
	virtual void render(Mesh* mesh, Matrix44 model, Camera * camera) = 0;
	virtual void renderInMenu() = 0;
};

class StandardMaterial : public Material {
public:

	StandardMaterial();
	~StandardMaterial();

	void setUniforms(Camera* camera, Matrix44 model);
	void render(Mesh* mesh, Matrix44 model, Camera * camera);
	void renderInMenu();
};

class WireframeMaterial : public StandardMaterial {
public:

	WireframeMaterial();
	~WireframeMaterial();

	void render(Mesh* mesh, Matrix44 model, Camera * camera);
};


class VRMaterial : public StandardMaterial //volume rendering material
{
public:
	Volume* volume = NULL;
	float step_Length;

	Texture* jittering = NULL;

	//LUT
	Texture* LUT = NULL; //LUT map
	float sl1; //slider 1 for manual LUT
	float sl2; //slider 2 for manual LUT

	//Gradient
	float th; //threshold
	float h; //h value
	Light* light;

	vec4 clipping; //clipping plane

	//ImGui
	bool jit_bool;
	bool part1;
	bool part2;
	bool part2_text;
	bool part2_sliders;
	bool gradient_option;


	VRMaterial(bool jit_bool_, bool part_1, bool part_2, bool part_2_text, bool part_2_sliders, bool gradient_option_, Shader* vrShad, Texture* textVolume, float num_Length, float sl_1, float sl_2, Texture* jit, Texture* lut, Light* light_, float th_, float h_, vec4 clipping_);
	~VRMaterial();

	void setUniforms(Camera* camera, Matrix44 model);
};


#endif