#include "glm-noise.h"
#include "glm/vec2.hpp"
#include "glm/geometric.hpp"
#include "glm/gtx/vec_swizzle.hpp"

static const unsigned char perm[512] = {
	151, 160, 137, 91, 90, 15,
	131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23,
	190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33,
	88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166,
	77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244,
	102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196,
	135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123,
	5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42,
	223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9,
	129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228,
	251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239, 107,
	49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254,
	138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180,
	// Repeat
	151, 160, 137, 91, 90, 15,
	131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23,
	190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33,
	88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166,
	77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244,
	102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196,
	135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123,
	5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42,
	223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9,
	129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228,
	251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239, 107,
	49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254,
	138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180
};
static const unsigned char perm12[256] = {
    7, 4, 5, 7,	6, 3,11, 1, 9,11, 0, 5, 2, 5, 7, 9,
    8, 0, 7, 6, 9,10, 8, 3, 1, 0, 9,10,11,10, 6, 4,
    7, 0, 6, 3, 0, 2, 5, 2,10, 0, 3,11, 9,11,11, 8,
    9, 9, 9, 4, 9, 5, 8, 3, 6, 8, 5, 4, 3, 0, 8, 7,
    2, 9,11, 2, 7, 0, 3,10, 5, 2, 2, 3,11, 3, 1, 2,
    0, 7, 1, 2, 4, 9, 8, 5, 7,10, 5, 4, 4, 6,11, 6,
    5, 1, 3, 5, 1, 0, 8, 1, 5, 4, 0, 7, 4, 5, 6, 1,
    8, 4, 3,10, 8, 8, 3, 2, 8, 4, 1, 6, 5, 6, 3, 4,
    4, 1,10,10, 4, 3, 5,10, 2, 3,10, 6, 3,10, 1, 8,
    3, 2,11,11,11, 4,10, 5, 2, 9, 4, 6, 7, 3, 2, 9,
    11,8, 8, 2, 8,10, 7,10, 5, 9, 5,11,11, 7, 4, 9,
    9,10, 3, 1, 7, 2, 0, 2, 7, 5, 8, 4,10, 5, 4, 8,
    2, 6, 1, 0,11,10, 2, 1,10, 6, 0, 0,11,11, 6, 1,
    9, 3, 1, 7, 9, 2,11,11, 1, 0,10, 7, 1, 7,10, 1,
    4, 0, 0, 8, 7, 1, 2, 9, 7, 4, 6, 2, 6, 8, 1, 9,
    6, 6, 7, 5, 0, 0, 3, 9, 8, 3, 6, 6,11, 1, 0, 0
};
static const float Gradients3D[3 * 12] = {
    1.f, 1.f, 0.f,
    -1.f, 1.f, 0.f,
    1.f, -1.f, 0.f,
    -1.f, -1.f, 0,
    1.f, 0.f, 1.f,
    -1.f, 0.f, 1.f,
    1.f, 0.f, -1.f,
    -1.f, 0.f, -1.f,
    0.f, 1.f, 1.f,
    0.f, -1.f, 1.f,
    0.f, 1.f, -1.f,
    0.f, -1.f, -1.f
};

static inline int permuteHash(int i)
{
    return perm[i & 0xFF];
}

static inline int permuteHashOffset(int i, int offset)
{
	return perm[(i & 0xFF) + (offset & 0xFF)];
}

static inline int Index3D(glm::ivec3 iP, int seed)
{
	return perm12[permuteHash(iP.x + permuteHash(iP.y + permuteHashOffset(iP.z, seed)))];
}

static inline float Grad3D(glm::vec3 p, int seed)
{
	const int gradIdx = Index3D(p, seed);
	glm::vec3 grad = glm::vec3(Gradients3D[gradIdx], Gradients3D[gradIdx + 1], Gradients3D[gradIdx + 2]);

	return glm::dot(p, grad);
}

static inline float Grad3D(glm::vec3 p, int seed, glm::vec3& grad)
{
    const int gradIdx = Index3D(p, seed);
	grad = glm::vec3(Gradients3D[gradIdx], Gradients3D[gradIdx+1], Gradients3D[gradIdx+2]);

	return glm::dot(p, grad);
}

// Skew/Unskew factors
#define F3 (1.0f / 3.0f)
#define G3 (1.0f / 6.0f)

float SimplexNoise3D(const glm::vec3 p, int seed)
{
	const glm::vec3 Skew = glm::vec3(F3, G3, 0.5);
	// First corner, skewed integer coord
	glm::vec3 pI = glm::floor(p + glm::dot(p, glm::xxx(Skew)));
	glm::vec3 surf0 = p - pI + glm::dot(pI, glm::yyy(Skew));

	// Other corners
	glm::vec3 g = glm::step(glm::yzx(surf0), glm::xyz(surf0)); // equivalent to (x < y), (y < z), (z < x)
	glm::vec3 l = 1.f - g; // inverts?
	// Simplex offsets (TODO, map this out for future reference?)
	glm::vec3 iSimplex1 = glm::min(g, l);
	glm::vec3 iSimplex2 = glm::max(g, l);

	// Surflet coords
	glm::vec3 surf1 = surf0 - iSimplex1 + glm::yyy(Skew); // G3
	glm::vec3 surf2 = surf0 - iSimplex2 + glm::xxx(Skew); // 2*(1/6) == (1/3) == F3
	glm::vec3 surf3 = surf0 -			  glm::zzz(Skew); // -1 + 3*(1/6) == -0.5

	glm::vec4 t = glm::max(0.6f - glm::vec4(glm::dot(surf0, surf0), glm::dot(surf1, surf1), glm::dot(surf2, surf2), glm::dot(surf3, surf3)), 0.f);
	t *= t;
	glm::vec4 grads = glm::vec4(Grad3D(surf0, seed), Grad3D(surf1, seed), Grad3D(surf2, seed), Grad3D(surf3, seed));

	// GLSL version uses 105 here?
	return 32.f * glm::dot(t*t, grads);
}

glm::vec4 SimplexNoise3DGrad(const glm::vec3 p, int seed)
{
	const glm::vec3 Skew = glm::vec3(F3, G3, 0.5);
	// First corner, skewed integer coord
	glm::vec3 pI = glm::floor(p + glm::dot(p, glm::xxx(Skew)));
	glm::vec3 surf0 = p - pI + glm::dot(pI, glm::yyy(Skew));

	// Other corners
	glm::vec3 g = glm::step(glm::yzx(surf0), glm::xyz(surf0)); // equivalent to (x < y), (y < z), (z < x)
	glm::vec3 l = 1.f - g; // inverts?
	// Simplex offsets (TODO, map this out for future reference?)
	glm::vec3 iSimplex1 = glm::min(g, l);
	glm::vec3 iSimplex2 = glm::max(g, l);

	// Surflet coords
	glm::vec3 surf1 = surf0 - iSimplex1 + glm::yyy(Skew); // G3
	glm::vec3 surf2 = surf0 - iSimplex2 + glm::xxx(Skew); // 2*(1/6) == (1/3) == F3
	glm::vec3 surf3 = surf0 - glm::zzz(Skew); // -1 + 3*(1/6) == -0.5

	glm::vec4 t = glm::max(0.6f - glm::vec4(glm::dot(surf0, surf0), glm::dot(surf1, surf1), glm::dot(surf2, surf2), glm::dot(surf3, surf3)), 0.f);
	glm::vec3 grad0, grad1, grad2, grad3;
	glm::vec4 grads = glm::vec4(Grad3D(surf0, seed, grad0), Grad3D(surf1, seed, grad1), Grad3D(surf2, seed, grad2), Grad3D(surf3, seed, grad3));

	//grad0 = glm::normalize(grad0);
	//grad1 = glm::normalize(grad1);
	//grad2 = glm::normalize(grad2);
	//grad3 = glm::normalize(grad3);

	glm::vec4 t2 = t * t;
	glm::vec4 t4 = t2*t2;

	// GLSL version uses 105 here?
	return 32.f * glm::dot(t2, grads);
}


//// Calculate simplex offsets
//glm::vec3 iSimplex1, iSimplex2;
//if (surf0.x > surf0.y)
//{
//	if (surf0.y >= surf0.z)
//	{
//		iSimplex1 = glm::vec3(1, 0, 0);
//		iSimplex2 = glm::vec3(1, 1, 0);
//	}
//	else if (surf0.x >= surf0.z)
//	{
//		iSimplex1 = glm::vec3(1, 0, 0);
//		iSimplex2 = glm::vec3(1, 0, 1);
//	}
//	else
//	{
//		iSimplex1 = glm::vec3(0, 0, 1);
//		iSimplex2 = glm::vec3(1, 0, 1);
//	}
//}
//else // x < y
//{
//	if (surf0.y < surf0.z)
//	{
//		iSimplex1 = glm::vec3(0, 0, 1);
//		iSimplex2 = glm::vec3(0, 1, 1);
//	}
//	else if (surf0.x < surf0.z)
//	{
//		iSimplex1 = glm::vec3(0, 1, 0);
//		iSimplex2 = glm::vec3(0, 1, 1);
//	}
//	else
//	{
//		iSimplex1 = glm::vec3(0, 1, 0);
//		iSimplex2 = glm::vec3(1, 1, 0);
//	}
//}
