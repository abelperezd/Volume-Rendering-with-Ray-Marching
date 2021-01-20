#pragma once

#include "framework.h"

//This class contains all the info about the properties of the light
class Light
{
public:
	Vector3 position; //where is the light
	Vector3 color;
	float intensity;

	Light();

	//possible method: uploads properties to shader uniforms
	//void uploadToShader(Shader* shader);
};
