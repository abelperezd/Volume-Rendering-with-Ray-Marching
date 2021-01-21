
#ifndef VOLUME_H
#define VOLUME_H

#include "includes.h"
#include "framework.h"

//Class to represent a volume
class Volume
{
public:
	unsigned int width;
	unsigned int height;
	unsigned int depth;

	float widthSpacing;
	float heightSpacing;
	float depthSpacing;

	unsigned int voxelBytes;	//1, 2 or 4
	unsigned int voxelChannels;	//1, 2, 3 or 4
	unsigned int voxelType;		//0: unsigned int, 1: int, 2: float, 3: other

	Uint8* data; //bytes with the pixel information

	Volume();
	Volume(unsigned int w, unsigned int h, unsigned int d, unsigned int channels = 1, unsigned int bytes = 1, unsigned int type = 0);
	~Volume();

	unsigned int getTextureFormat();
	unsigned int getTextureType();
	unsigned int getTextureInternalFormat();

	void resize(int w, int h, int d, unsigned int channels = 1, unsigned int bytes = 1);
	void clear();

	bool loadVL(const char* filename);
	bool loadPVM(const char* filename);

	//Slow methods
	void fillSphere();
	void fillNoise(float frequency, int octaves, unsigned int seed, unsigned int channel = 1); //Channel 1 for R to 4 for A
	void fillWorleyNoise(unsigned int cellsPerSide = 4, unsigned int channel = 1); //Channel 1 for R to 4 for A
};

#endif