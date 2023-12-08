#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include "svdqef.h"
#include "glm/gtx/vec_swizzle.hpp"

// sym == symmetric?

void givensCoeffsSym(const float a_pp, const float a_pq, const float a_qq, float& c, float & s)
{
	if (a_pq == 0.f)
	{
		c = 1.f;
		s = 0.f;
		return;
	}
	const float tau = (a_qq - a_pp) / (2.f * a_pq);
	const float stt = glm::sqrt(1.f + tau*tau);
	const float tan = 1.f / ((tau >= 0.f) ? (tau + stt) : (tau - stt));
	c = glm::inversesqrt(1.f + tan*tan);
	s = tan * c;
}

void svdRotateXY(float& x, float& y, const float c, const float s)
{
	const float u = x;
	const float v = y;
	x = c*u - s*v;
	y = s*u + c*v;
}

// What does the Q mean here?
void svdRotateQXY(float& x, float& y, float& a, const float c, const float s)
{
	const float cc = c*c; 
	const float ss = s*s;
	const float mx = 2.f * c * s * a;
	const float u = x;
	const float v = y;
	x = cc*u - mx + ss*v;
	y = ss*u + mx + cc*v;
}

void svdRotate(glm::mat3& vtav, glm::mat3& v, const int a, const int b)
{
	if (vtav[a][b] == 0.0)
		return;

	float c, s;
	givensCoeffsSym(vtav[a][a], vtav[a][b], vtav[b][b], c, s);
	svdRotateQXY(vtav[a][a], vtav[b][b], vtav[a][b], c, s);
	svdRotateXY(vtav[0][3-b], vtav[1-a][2], c, s);
	vtav[a][b] = 0.f;

	svdRotateXY(v[0][a], v[0][b], c, s);
	svdRotateXY(v[1][a], v[1][b], c, s);
	svdRotateXY(v[2][a], v[2][b], c, s);
}

#define SVD_NUM_SWEEPS 5
void svdSolveSym(glm::mat3 a, glm::vec3& sigma, glm::mat3& v)
{
	// Assuming that A is symmetric: can optimise all operations for the upper right triagonal
	//glm::mat3 vtav = a;
	// Assuming V is identity: you can also pass a matrix the rotations should be applied to
	// U is not computed
	for (int i = 0; i < 5; ++i)
	{
		svdRotate(a, v, 0, 1);
		svdRotate(a, v, 0, 2);
		svdRotate(a, v, 1, 2);
	}
	sigma = glm::vec3(a[0][0], a[1][1], a[2][2]);
}

// tol?
float svdInvDet(const float x, const float tol)
{
	return (abs(x) < tol || abs(1.f / x) < tol) ? 0.f : (1.f / x);
}

// o == output?
void svdPseudoInverse(glm::mat3& o, const glm::vec3& sigma, const glm::mat3& v)
{
	#define TINY_NUMBER 1e-20f
	const float d0 = svdInvDet(sigma[0], TINY_NUMBER);
	const float d1 = svdInvDet(sigma[1], TINY_NUMBER);
	const float d2 = svdInvDet(sigma[2], TINY_NUMBER);
	o = glm::mat3(
		/*[0][0]*/ v[0][0] * d0 * v[0][0] + v[0][1] * d1 * v[0][1] + v[0][2] * d2 * v[0][2],
		/*[0][1]*/ v[0][0] * d0 * v[1][0] + v[0][1] * d1 * v[1][1] + v[0][2] * d2 * v[1][2],
		/*[0][2]*/ v[0][0] * d0 * v[2][0] + v[0][1] * d1 * v[2][1] + v[0][2] * d2 * v[2][2],
		/*[1][0]*/ v[1][0] * d0 * v[0][0] + v[1][1] * d1 * v[0][1] + v[1][2] * d2 * v[0][2],
		/*[1][1]*/ v[1][0] * d0 * v[1][0] + v[1][1] * d1 * v[1][1] + v[1][2] * d2 * v[1][2],
		/*[1][2]*/ v[1][0] * d0 * v[2][0] + v[1][1] * d1 * v[2][1] + v[1][2] * d2 * v[2][2],
		/*[2][0]*/ v[2][0] * d0 * v[0][0] + v[2][1] * d1 * v[0][1] + v[2][2] * d2 * v[0][2],
		/*[2][1]*/ v[2][0] * d0 * v[1][0] + v[2][1] * d1 * v[1][1] + v[2][2] * d2 * v[1][2],
		/*[2][2]*/ v[2][0] * d0 * v[2][0] + v[2][1] * d1 * v[2][1] + v[2][2] * d2 * v[2][2]
	);
}

void svdSolveATA_ATb(const glm::mat3& ATA, const glm::vec3 ATb, glm::vec3 x)
{
	glm::mat3 V(1.f);
	glm::vec3 sigma;
	svdSolveSym(ATA, sigma, V);

	glm::mat3 Vinv;
	svdPseudoInverse(Vinv, sigma, V);
	x = Vinv * ATb;
}

glm::vec3 svdVmulSym(const glm::mat3& a, const glm::vec3& v)
{
	return glm::vec3(
		dot(a[0], v), 
		(a[0][1] * v.x) + (a[1][1] * v.x) + (a[1][2] * v.z),
		(a[0][2] * v.x) + (a[1][2] * v.y) + (a[2][2] * v.z)
	);
}

void svdMulATASym(glm::mat3& o, const glm::mat3& a)
{
	o[0][0] = a[0][0] * a[0][0] + a[1][0] * a[1][0] + a[2][0] * a[2][0];
	o[0][1] = a[0][0] * a[0][1] + a[1][0] * a[1][1] + a[2][0] * a[2][1];
	o[0][2] = a[0][0] * a[0][2] + a[1][0] * a[1][2] + a[2][0] * a[2][2];
	o[1][1] = a[0][1] * a[0][1] + a[1][1] * a[1][1] + a[2][1] * a[2][1];
	o[1][2] = a[0][1] * a[0][2] + a[1][1] * a[1][2] + a[2][1] * a[2][2];
	o[2][2] = a[0][2] * a[0][2] + a[1][2] * a[1][2] + a[2][2] * a[2][2];
}

void svdSolveAxB(const glm::mat3& a, const glm::vec3& b, glm::mat3& ATA, glm::vec3& ATb, glm::vec3& x)
{
	svdMulATASym(ATA, a);
	ATb = b * a; // transpose(a) * b
	svdSolveATA_ATb(ATA, ATb, x);
}

void qefAdd(const glm::vec3& n, const glm::vec3& p, glm::mat3& ATA, glm::vec3& ATb, glm::vec4& pointAccumulate)
{
	ATA[0][0] += n.x * n.x;
	ATA[0][1] += n.x * n.y;
	ATA[0][2] += n.x * n.z;
	ATA[1][1] += n.y * n.y;
	ATA[1][2] += n.y * n.z;
	ATA[2][2] += n.z * n.z;

	const float b = dot(p, n);
	ATb += n * b;
	pointAccumulate += glm::vec4(p, 1.f);
}

float qefCalcError(const glm::mat3& A, const glm::vec3& x, const glm::vec3& b)
{
	glm::vec3 tmp = b - svdVmulSym(A, x);
	return dot(tmp, tmp);
}

// x output is feature point
float qefSolve(const glm::mat3& ATA, glm::vec3 ATb, const glm::vec4& pointAccumulate, glm::vec3& x)
{
	const glm::vec3 masspoint = glm::xyz(pointAccumulate) / pointAccumulate.w;
	ATb -= svdVmulSym(ATA, masspoint);
	svdSolveATA_ATb(ATA, ATb, x);
	const float result = qefCalcError(ATA, x, ATb);

	x += masspoint;

	return result;
}
