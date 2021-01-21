
#include "pvmparser.h"

#include <sstream>

#define DDS_MAXSTR (256)

#define DDS_BLOCKSIZE (1<<20)
#define DDS_INTERLEAVE (1<<24)

#define DDS_RL (7)

#define DDS_ISINTEL (*((unsigned char *)(&DDS_INTEL)+1)==0)

// read from a RAW file
unsigned char *readfiled(FILE *file, long long *bytes, const long long blocksize = 1 << 20)
{
	unsigned char *data, *data2;
	long long cnt, blkcnt;

	data = NULL;
	*bytes = cnt = 0;

	do
	{
		if (data == NULL)
		{
			if ((data = (unsigned char *)malloc(blocksize)) == NULL) return(NULL);
		}
		else
		{
			if ((data2 = (unsigned char *)realloc(data, cnt + blocksize)) == NULL) { free(data); return(NULL); }
			data = data2;
		}

		blkcnt = fread(&data[cnt], 1, blocksize, file);
		cnt += blkcnt;
	} while (blkcnt == blocksize);

	if (cnt == 0)
	{
		free(data);
		return(NULL);
	}

	if ((data2 = (unsigned char *)realloc(data, cnt)) == NULL) { free(data); return(NULL); }
	else data = data2;

	*bytes = cnt;

	return(data);
}

unsigned char *DDS_cache;
unsigned int DDS_cachepos, DDS_cachesize;

unsigned int DDS_buffer;
unsigned int DDS_bufsize;

unsigned short int DDS_INTEL = 1;


unsigned int DDS_shiftl(const unsigned int value, const unsigned int bits)
{
	return((bits >= 32) ? 0 : value << bits);
}

unsigned int DDS_shiftr(const unsigned int value, const unsigned int bits)
{
	return((bits >= 32) ? 0 : value >> bits);
}

void DDS_swap4(char *x)
{
	char a = x[0];
	char b = x[1];
	char c = x[2];
	char d = x[3];

	x[0] = d;
	x[1] = c;
	x[2] = b;
	x[3] = a;
}

void DDS_initbuffer()
{
	DDS_buffer = 0;
	DDS_bufsize = 0;
}

void DDS_clearbits()
{
	DDS_cache = NULL;
	DDS_cachepos = 0;
	DDS_cachesize = 0;
}

void DDS_savebits(unsigned char **data, unsigned int *size)
{
	*data = DDS_cache;
	*size = DDS_cachepos;
}

void DDS_loadbits(unsigned char *data, unsigned int size)
{
	DDS_cache = data;
	DDS_cachesize = size;

	if ((DDS_cache = (unsigned char *)realloc(DDS_cache, DDS_cachesize + 4)) == NULL) return;
	*((unsigned int *)&DDS_cache[DDS_cachesize]) = 0;

	DDS_cachesize = 4 * ((DDS_cachesize + 3) / 4);
	if ((DDS_cache = (unsigned char *)realloc(DDS_cache, DDS_cachesize)) == NULL) return;
}

unsigned int DDS_readbits(unsigned int bits)
{
	unsigned int value;

	if (bits < DDS_bufsize)
	{
		DDS_bufsize -= bits;
		value = DDS_shiftr(DDS_buffer, DDS_bufsize);
	}
	else
	{
		value = DDS_shiftl(DDS_buffer, bits - DDS_bufsize);

		if (DDS_cachepos >= DDS_cachesize) DDS_buffer = 0;
		else
		{
			DDS_buffer = *((unsigned int *)&DDS_cache[DDS_cachepos]);
			if (DDS_ISINTEL) DDS_swap4((char *)&DDS_buffer);
			DDS_cachepos += 4;
		}

		DDS_bufsize += 32 - bits;
		value |= DDS_shiftr(DDS_buffer, DDS_bufsize);
	}

	DDS_buffer &= DDS_shiftl(1, DDS_bufsize) - 1;

	return(value);
}

int DDS_code(int bits)
{
	return(bits > 1 ? bits - 1 : bits);
}

int DDS_decode(int bits)
{
	return(bits >= 1 ? bits + 1 : bits);
}

// deinterleave a byte stream
void DDS_deinterleave(unsigned char *data, unsigned int bytes,
	unsigned int skip, unsigned int block = 0,
	bool restore = false)
{
	unsigned int i, j, k;

	unsigned char *data2, *ptr;

	if (skip <= 1) return;

	if (block == 0)
	{
		if ((data2 = (unsigned char *)malloc(bytes)) == NULL) return;

		if (!restore)
			for (ptr = data2, i = 0; i < skip; i++)
				for (j = i; j < bytes; j += skip) *ptr++ = data[j];
		else
			for (ptr = data, i = 0; i < skip; i++)
				for (j = i; j < bytes; j += skip) data2[j] = *ptr++;

		memcpy(data, data2, bytes);
	}
	else
	{
		if ((data2 = (unsigned char *)malloc((bytes < skip*block) ? bytes : skip * block)) == NULL) return;

		if (!restore)
		{
			for (k = 0; k < bytes / skip / block; k++)
			{
				for (ptr = data2, i = 0; i < skip; i++)
					for (j = i; j < skip*block; j += skip) *ptr++ = data[k*skip*block + j];

				memcpy(data + k * skip*block, data2, skip*block);
			}

			for (ptr = data2, i = 0; i < skip; i++)
				for (j = i; j < bytes - k * skip*block; j += skip) *ptr++ = data[k*skip*block + j];

			memcpy(data + k * skip*block, data2, bytes - k * skip*block);
		}
		else
		{
			for (k = 0; k < bytes / skip / block; k++)
			{
				for (ptr = data + k * skip*block, i = 0; i < skip; i++)
					for (j = i; j < skip*block; j += skip) data2[j] = *ptr++;

				memcpy(data + k * skip*block, data2, skip*block);
			}

			for (ptr = data + k * skip*block, i = 0; i < skip; i++)
				for (j = i; j < bytes - k * skip*block; j += skip) data2[j] = *ptr++;

			memcpy(data + k * skip*block, data2, bytes - k * skip*block);
		}
	}

	free(data2);
}

// interleave a byte stream
void DDS_interleave(unsigned char *data, unsigned int bytes,
	unsigned int skip, unsigned int block = 0)
{
	DDS_deinterleave(data, bytes, skip, block, true);
}


// decode a Differential Data Stream
void DDS_decode(unsigned char *chunk, unsigned int size,
	unsigned char **data, unsigned int *bytes,
	unsigned int block)
{
	unsigned int skip, strip;

	unsigned char *ptr1, *ptr2;

	unsigned int cnt, cnt1, cnt2;
	int bits, act;

	DDS_initbuffer();

	DDS_clearbits();
	DDS_loadbits(chunk, size);

	skip = DDS_readbits(2) + 1;
	strip = DDS_readbits(16) + 1;

	ptr1 = ptr2 = NULL;
	cnt = act = 0;

	while ((cnt1 = DDS_readbits(DDS_RL)) != 0)
	{
		bits = DDS_decode(DDS_readbits(3));

		for (cnt2 = 0; cnt2 < cnt1; cnt2++)
		{
			if (strip == 1 || cnt <= strip) act += DDS_readbits(bits) - (1 << bits) / 2;
			else act += *(ptr2 - strip) - *(ptr2 - strip - 1) + DDS_readbits(bits) - (1 << bits) / 2;

			while (act < 0) act += 256;
			while (act > 255) act -= 256;

			if ((cnt&(DDS_BLOCKSIZE - 1)) == 0)
				if (ptr1 == NULL)
				{
					if ((ptr1 = (unsigned char *)malloc(DDS_BLOCKSIZE)) == NULL) return;
					ptr2 = ptr1;
				}
				else
				{
					if ((ptr1 = (unsigned char *)realloc(ptr1, cnt + DDS_BLOCKSIZE)) == NULL) return;
					ptr2 = &ptr1[cnt];
				}

			*ptr2++ = act;
			cnt++;
		}
	}

	if (ptr1 != NULL)
		if ((ptr1 = (unsigned char *)realloc(ptr1, cnt)) == NULL) return;

	DDS_interleave(ptr1, cnt, skip, block);

	*data = ptr1;
	*bytes = cnt;
}


unsigned char *parsePVM(const char *filename, unsigned int *width, unsigned int *height, unsigned int *depth, unsigned int *components, float *scalex, float *scaley, float *scalez) {
	unsigned int version = 1;

	FILE* file;
	if ((file = fopen(filename, "rb")) == NULL) return(NULL);

	char type[4];
	unsigned char *volume, *chunk, *data, *ptr;
	long long size;
	unsigned int bytes, numc;

	float sx = 1.0f, sy = 1.0f, sz = 1.0f;
	unsigned int len1 = 0, len2 = 0, len3 = 0, len4 = 0;

	type[3] = '\0';
	fread(type, 1, 3, file);

	if (strcmp(type, "PVM") == 0) {
		rewind(file);
		data = readfiled(file, &size);
		fclose(file);
	}
	else if(strcmp(type, "DDS") == 0) {
		fgetc(file); //skip space
		fread(type, 1, 3, file);
		if (strcmp(type, "v3d") == 0) version = 0;
		else if (strcmp(type, "v3e") == 0) version = DDS_INTERLEAVE;
		else {
			fclose(file);
			return NULL;
		}
		fgetc(file); //skip \n
		chunk = readfiled(file, &size);
		fclose(file);
		DDS_decode(chunk, (unsigned int)size, &data, &bytes, version);
		free(chunk);
	}
	else {
		fclose(file);
		return NULL;
	}

	if ((data = (unsigned char *)realloc(data, bytes + 1)) == NULL) return NULL;
	data[bytes] = '\0';

	if (strncmp((char *)data, "PVM\n", 4) != 0)
	{
		if (strncmp((char *)data, "PVM2\n", 5) == 0) version = 2;
		else if (strncmp((char *)data, "PVM3\n", 5) == 0) version = 3;
		else return(NULL);

		ptr = &data[5];
		if (sscanf((char *)ptr, "%d %d %d\n%g %g %g\n", width, height, depth, &sx, &sy, &sz) != 6) return NULL;
		if (*width < 1 || *height < 1 || *depth < 1 || sx <= 0.0f || sy <= 0.0f || sz <= 0.0f) return NULL;
		ptr = (unsigned char *)strchr((char *)ptr, '\n') + 1;
	}
	else
	{
		ptr = &data[4];
		while (*ptr == '#')
			while (*ptr++ != '\n');

		if (sscanf((char *)ptr, "%d %d %d\n", width, height, depth) != 3) return NULL;
		if (*width < 1 || *height < 1 || *depth < 1) return NULL;
	}

	if (scalex != NULL && scaley != NULL && scalez != NULL)
	{
		*scalex = sx;
		*scaley = sy;
		*scalez = sz;
	}

	ptr = (unsigned char *)strchr((char *)ptr, '\n') + 1;
	if (sscanf((char *)ptr, "%d\n", &numc) != 1) return NULL;
	if (numc < 1) return NULL;

	if (components != NULL) *components = numc;
	else if (numc != 1) return NULL;

	ptr = (unsigned char *)strchr((char *)ptr, '\n') + 1;
	if (version == 3) len1 = strlen((char *)(ptr + (*width)*(*height)*(*depth)*numc)) + 1;
	if (version == 3) len2 = strlen((char *)(ptr + (*width)*(*height)*(*depth)*numc + len1)) + 1;
	if (version == 3) len3 = strlen((char *)(ptr + (*width)*(*height)*(*depth)*numc + len1 + len2)) + 1;
	if (version == 3) len4 = strlen((char *)(ptr + (*width)*(*height)*(*depth)*numc + len1 + len2 + len3)) + 1;
	if ((volume = (unsigned char *)malloc((*width)*(*height)*(*depth)*numc + len1 + len2 + len3 + len4)) == NULL) return NULL;
	if (data + bytes != ptr + (*width)*(*height)*(*depth)*numc + len1 + len2 + len3 + len4) return NULL;

	memcpy(volume, ptr, (*width)*(*height)*(*depth)*numc + len1 + len2 + len3 + len4);
	free(data);

	return(volume);
}