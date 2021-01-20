#include "material.h"
#include "texture.h"
#include "application.h"

StandardMaterial::StandardMaterial()
{
	color = vec4(1.f, 1.f, 1.f, 1.f);
	shader = Shader::Get("data/shaders/basic.vs", "data/shaders/flat.fs");
} StandardMaterial::~StandardMaterial(){}

VRMaterial::VRMaterial(bool jit_bool_, bool part_1, bool part_2, bool part_2_text, bool part_2_sliders, bool gradient_option_, Shader* vrShad, Texture* textVolume, float num_Length, float sl_1, float sl_2, Texture* jit, Texture* lut, Light* light_, float th_, float h_, vec4 clipping_) {
	step_Length = num_Length;
	shader = vrShad;
	texture = textVolume; //volume texture
	jittering = jit; //Jittering map

	//LUT
	LUT = lut; //LUT map
	sl1 = sl_1; //slider 1 for manual LUT
	sl2 = sl_2; //slider 2 for manual LUT
	
	//Gradient
	th = th_; //threshold
	h = h_; //h value
	light = light_;

	clipping = clipping_; //clipping plane

	//ImGui
	jit_bool = jit_bool_;
	part1 = part_1;
	part2 = part_2;
	part2_text = part_2_text;
	part2_sliders = part_2_sliders;
	gradient_option = gradient_option_;
}

void VRMaterial::setUniforms(Camera* camera, Matrix44 model) {
	shader->setUniform("u_viewprojection", camera->viewprojection_matrix);
	shader->setUniform("u_camera_position", camera->eye);
	shader->setUniform("u_model", model);
	shader->setUniform("u_time", Application::instance->time);

	shader->setUniform("u_color", color);

	shader->setUniform("u_texture", texture);
	shader->setUniform("jittering", jittering);
	shader->setUniform("lut", LUT);
	
	if (light) {
		shader->setUniform("light_pos", light->position);
		shader->setUniform("light_color", light->color);
		shader->setUniform("light_intensity", light->intensity);
	}
	shader->setUniform("th", th);
	shader->setUniform("h", h);
	shader->setUniform("clipping", clipping);

	shader->setUniform("step_Length", step_Length);
	shader->setUniform("sl1", sl1);
	shader->setUniform("sl2", sl2);

	shader->setUniform("jitBool", jit_bool);
	shader->setUniform("part1", part1);
	shader->setUniform("part2", part2);

	shader->setUniform("part2_text", part2_text);
	shader->setUniform("part2_sliders", part2_sliders);
	shader->setUniform("gradient_option", gradient_option);
}

void StandardMaterial::setUniforms(Camera* camera, Matrix44 model)
{
	//upload node uniforms
	shader->setUniform("u_viewprojection", camera->viewprojection_matrix);
	shader->setUniform("u_camera_position", camera->eye);
	shader->setUniform("u_model", model);
	shader->setUniform("u_time", Application::instance->time);

	shader->setUniform("u_color", color);

	if (texture)
		shader->setUniform("u_texture", texture);
}

void StandardMaterial::render(Mesh* mesh, Matrix44 model, Camera* camera)
{
	//set flags
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	if (mesh && shader)
	{
		//enable shader
		shader->enable();

		//upload uniforms
		setUniforms(camera, model);

		//do the draw call
		mesh->render(GL_TRIANGLES);

		//disable shader
		shader->disable();
	}
}

void StandardMaterial::renderInMenu()
{
	ImGui::ColorEdit3("Color", (float*)&color); // Edit 3 floats representing a color
}

WireframeMaterial::WireframeMaterial()
{
	color = vec4(1.f, 1.f, 1.f, 1.f);
	shader = Shader::Get("data/shaders/basic.vs", "data/shaders/flat.fs");
}

WireframeMaterial::~WireframeMaterial()
{

}

void WireframeMaterial::render(Mesh* mesh, Matrix44 model, Camera * camera)
{
	if (shader && mesh)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		//enable shader
		shader->enable();

		//upload material specific uniforms
		setUniforms(camera, model);

		//do the draw call
		mesh->render(GL_TRIANGLES);

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}
