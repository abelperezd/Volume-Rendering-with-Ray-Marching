#include "volume.h"
#include "extra/pvmparser.h"
#include "extra/PerlinNoise.hpp"

Volume::Volume() {
	width = height = depth = 0;
	widthSpacing = heightSpacing = depthSpacing = 1.0; 
	data = NULL;
	voxelChannels = 1; 
	voxelBytes = 1;
	voxelType = 0;
}

Volume::Volume(unsigned int w, unsigned int h, unsigned int d, unsigned int channels, unsigned int bytes, unsigned int type) {
	widthSpacing = heightSpacing = depthSpacing = 1.0;
	voxelType = type;
	data = NULL;
	resize(w, h, d, channels, bytes);
}

Volume::~Volume() {
	if (data) delete[]data;
	data = NULL;
}

void Volume::resize(int w, int h, int d, unsigned int channels, unsigned int bytes) {
	if (data) delete[] data;
	width = w;
	height = h;
	depth = d;
	voxelChannels = channels;
	voxelBytes = bytes;
	data = new Uint8[w*h*d*channels*bytes];
	memset(data, 0, w*h*d*channels*bytes);
}

void Volume::clear() {
	if (data) delete[]data;
	data = NULL;
	width = height = depth = 0;
}

bool Volume::loadVL(const char* filename){
	FILE * file = fopen(filename, "rb");
	if (file == NULL)
	{
		return false;
	}

	GLuint version;
	fread(&version, 1, 4, file);
	if (version == 1)
	{
		fread(&width, 1, 4, file);
		fread(&height, 1, 4, file);
		fread(&depth, 1, 4, file);
		fread(&widthSpacing, 1, 4, file);
		fread(&heightSpacing, 1, 4, file);
		fread(&depthSpacing, 1, 4, file);
		fread(&voxelChannels, 1, 4, file);
		fread(&voxelBytes, 1, 4, file) / 8;
		voxelType = 0; //This version does not contain this value, we assume it's unsigned
	}
	if (version == 2)
	{
		fread(&width, 1, 4, file);
		fread(&height, 1, 4, file);
		fread(&depth, 1, 4, file);
		fread(&widthSpacing, 1, 4, file);
		fread(&heightSpacing, 1, 4, file);
		fread(&depthSpacing, 1, 4, file);
		fread(&voxelChannels, 1, 4, file);
		fread(&voxelBytes, 1, 4, file) / 8;
		fread(&voxelType, 1, 4, file);
	}
	else
	{
		std::cerr << "Version not supported: " << version << std::endl;
		fclose(file);
		return false;
	}

	resize(width, height, depth, voxelChannels, voxelBytes);
	fread(data, voxelBytes, width*height*depth*voxelChannels*voxelBytes, file);

	fclose(file);
	return true;
}

// http://paulbourke.net/dataformats/pvm/
// samples: http://schorsch.efi.fh-nuernberg.de/data/volume/
bool Volume::loadPVM(const char* filename){
	data = parsePVM(filename, &width, &height, &depth, &voxelChannels, &widthSpacing, &heightSpacing, &depthSpacing);
	voxelBytes = sizeof(data) / (width * height * depth * voxelChannels);

	if (data == NULL) return false;
	return true;
}

unsigned int Volume::getTextureFormat(){
	unsigned int format = GL_RED;
	switch (voxelChannels) {
	case 1:
		format = GL_RED;
		break;
	case 2:
		format = GL_RG;
		break;
	case 3:
		format = GL_RGB;
		break;
	case 4:
		format = GL_RGBA;
		break;
	}
	return format;
}

unsigned int Volume::getTextureType(){
	unsigned int type = GL_UNSIGNED_BYTE;
	switch (voxelType) {
	case 0: //unsigned
		switch (voxelBytes) {
		case 1: type = GL_UNSIGNED_BYTE; break;
		case 2: type = GL_UNSIGNED_SHORT; break;
		case 4: type = GL_UNSIGNED_INT; break;
		}
		break;
	case 1: //signed
		switch (voxelBytes) {
		case 1: type = GL_BYTE; break;
		case 2: type = GL_SHORT; break;
		case 4: type = GL_INT; break;
		}
		break;
	case 2: //float
		switch (voxelBytes) {
		case 2: type = GL_HALF_FLOAT; break;
		case 4: type = GL_FLOAT; break;
		}
		break;
	}
	return type;
}

unsigned int Volume::getTextureInternalFormat(){
	return getTextureFormat();
}

void Volume::fillSphere() {
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			for (int k = 0; k < depth; k++) {
				float f = 0;
				float x = 2.0*(((float)i / width) - 0.5);
				float y = 2.0*(((float)j / height) - 0.5);
				float z = 2.0*(((float)k / depth) - 0.5);

				f = (1.0 - (x*x + y * y + z * z) / 3.0);
				f = f < 0.5 ? 0.0 : f;

				data[i + width * j + width * height*k] = (Uint8)(f* 255.0);
			}
		}
	}
}

void Volume::fillNoise(float frequency, int octaves, unsigned int seed, unsigned int channel) {
	float f = frequency > 0.1 ? frequency < 64.0 ? frequency : 64.0 : 0.1;
	int o = octaves > 1 ? octaves < 16 ? octaves : 16 : 1;

	const siv::PerlinNoise perlin(seed);
	const float fx = (float)width / f;
	const float fy = (float)height / f;
	const float fz = (float)depth / f;


	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			for (int k = 0; k < depth; k++) {
				float v = perlin.octaveNoise0_1(i / fx, j / fy, k / fz, o);
				unsigned int index = i + j * width + k * width*height;
				data[(index * voxelChannels) + (channel - 1)] = (Uint8)(255 * v);
			}
		}
	}
}

void Volume::fillWorleyNoise(unsigned int cellsPerSide, unsigned int channel) {
	if (width != height || width != depth || width % cellsPerSide != 0) {
		std::cout << "Could not fill volume with Worley noise: All dimensions should be the same and divisible by cellsPerSide.\n";
		return;
	}
	if (channel > voxelChannels) {
		std::cout << "Could not fill volume with Worley noise: The volume doesn't have that numer of channels.\n";
		return;
	}
	
	unsigned int side = width;

	unsigned int cells = cellsPerSide;
	unsigned int subside = side / cells;

	unsigned int pointsCount = pow(cells, 3);
	vec3 *points = new vec3[pointsCount];

	float* _distances = new float[side*side*side];

	//Compute a relative point for each cell
	for (unsigned int i = 0; i < cells; i++) {
		for (unsigned int j = 0; j < cells; j++) {
			for (unsigned int k = 0; k < cells; k++) {
				unsigned int index = i + j * cells + k * cells * cells;
				points[index].random(0.5);
				points[index] = points[index] + vec3(0.5, 0.5, 0.5); //Random between 0 and 1
			}
		}
	}

	//Compute min distance to point and store the max on for normalization
	float maxdist = -1;
	for (unsigned int i = 0; i < side; i++) {
		for (unsigned int j = 0; j < side; j++) {
			for (unsigned int k = 0; k < side; k++) {
				vec3 point((float)i/subside, (float)j/subside, (float)k/subside);
				vec3 basep2(std::floor(point.x), std::floor(point.y), std::floor(point.z));
				float mindist = 10000;

				for (unsigned int dx = 0; dx < 3; dx++) {
					for (unsigned int dy = 0; dy < 3; dy++) {
						for (unsigned int dz = 0; dz < 3; dz++) {
							vec3 p2(basep2.x + dx - 1, basep2.y + dy - 1, basep2.z + dz - 1);
							vec3 wrappedp2(p2.x == -1 ? cells - 1 : p2.x == cells ? 0 : p2.x,
											p2.y == -1 ? cells - 1 : p2.y == cells ? 0 : p2.y,
											p2.z == -1 ? cells - 1 : p2.z == cells ? 0 : p2.z);
							unsigned int wrappedindex2 = wrappedp2.x + wrappedp2.y * cells + wrappedp2.z * cells * cells;
							vec3 point2 = points[wrappedindex2] + p2;

							float dist = point.distance(point2);
							if (dist < mindist) mindist = dist;
						}
					}
				}
				_distances[i + j*side + k*side*side] = mindist;
				if (mindist > maxdist) maxdist = mindist;
			}
		}
	}

	//Normalize and store
	for (unsigned int i = 0; i < side; i++) {
		for (unsigned int j = 0; j < side; j++) {
			for (unsigned int k = 0; k < side; k++) {
				unsigned int index = i + j*side + k * side*side;
				Uint8 v = std::floor( (_distances[index] / maxdist) * 256 );
				data[(index * voxelChannels) + (channel - 1)] = 256 - v;
			}
		}
	}

	delete _distances;
}