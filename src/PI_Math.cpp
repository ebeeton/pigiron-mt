// PigIron math implementation.
//
// Copyright Evan Beeton 2/1/2005

#include <cmath>
#include <map>
using std::map;
using std::pair;

using std::greater;

#include "PI_Math.h"
#include "PI_Geom.h"

float PI_Vec2::magnitude(void) const
{
	return sqrt(x * x + y * y);
}

void PI_Vec2::normalize(void)
{
	float scaleFactor = 1.0f / sqrt(x * x + y * y);
	x *= scaleFactor, y *= scaleFactor;
}


float PI_Vec3::Magnitude(void) const
{
	return sqrt(x * x + y * y + z * z);
}

void PI_Vec3::Normalize(void)
{
	float scaleFactor = 1.0f / sqrt(x * x + y * y + z * z);
	x *= scaleFactor, y *= scaleFactor, z *= scaleFactor;
}

// Rotate this vector by the transposed rotational components of m.
PI_Vec3 &PI_Vec3::TransposedRotate(const PI_Mat44 &m)
{
	PI_Vec3 temp = *this;

	x = m.mat[0] * temp.x + m.mat[1] * temp.y + m.mat[2] * temp.z;
	y = m.mat[4] * temp.x + m.mat[5] * temp.y + m.mat[6] * temp.z;
	z = m.mat[8] * temp.x + m.mat[9] * temp.y + m.mat[10] * temp.z;
	return *this;
}

// Rotate this vector by the rotational components of m.
PI_Vec3 &PI_Vec3::Rotate(const PI_Mat44 &m)
{
	PI_Vec3 temp = *this;

	x = m.mat[0] * temp.x + m.mat[4] * temp.y + m.mat[8] * temp.z;
	y = m.mat[1] * temp.x + m.mat[5] * temp.y + m.mat[9] * temp.z;
	z = m.mat[2] * temp.x + m.mat[6] * temp.y + m.mat[10] * temp.z;
	return *this;
}

// Constructor for building a rotation matrix around an arbitrary axis.
PI_Mat44::PI_Mat44(const PI_Vec3 &axis, float angleR)
{
	SetIdentity();
	MakeRotationMatrix(axis, angleR);
}

// This function only manipulates the 3x3 rotational components of the matrix.
void PI_Mat44::MakeRotationMatrix(const PI_Vec3 &axis, float angleRadians)
{
	float c = cos(angleRadians), s = sin(angleRadians), omc = 1 - c;

	// Column 1.
	mat[0] = c + axis.x * axis.x * omc;
	mat[1] = axis.x * axis.y * omc + axis.z * s;
	mat[2] = axis.x * axis.z * omc - axis.y * s;

	// Column 2.
	mat[4] = axis.x * axis.y * omc - axis.z * s;
	mat[5] = c + axis.y * axis.y * omc;
	mat[6] = axis.y * axis.z * omc + axis.x * s;

	// Column 3.
	mat[8] = axis.x * axis.z * omc + axis.y * s;
	mat[9] = axis.y * axis.z * omc - axis.x * s;
	mat[10] = c + axis.z * axis.z * omc;
}

// Copy the rotation components of another matrix into this one.
PI_Mat44 &PI_Mat44::CopyRotation(const PI_Mat44 &m)
{
	mat[0] = m.mat[0];
	mat[1] = m.mat[1];
	mat[2] = m.mat[2];

	mat[4] = m.mat[4];
	mat[5] = m.mat[5];
	mat[6] = m.mat[6];

	mat[8] = m.mat[8];
	mat[9] = m.mat[9];
	mat[10] = m.mat[10];

	return *this;
}

PI_Mat44 PI_Mat44::operator*(const PI_Mat44 &m) const
{
	PI_Mat44 prod(false);
	
	// Column 1
	prod.mat[0]  = mat[0] * m.mat[0] + mat[4] * m.mat[1] + mat[8] * m.mat[2] + mat[12] * m.mat[3];
	prod.mat[1]  = mat[1] * m.mat[0] + mat[5] * m.mat[1] + mat[9] * m.mat[2] + mat[13] * m.mat[3];
	prod.mat[2]  = mat[2] * m.mat[0] + mat[6] * m.mat[1] + mat[10] * m.mat[2] + mat[14] * m.mat[3];
	prod.mat[3]  = mat[3] * m.mat[0] + mat[7] * m.mat[1] + mat[11] * m.mat[2] + mat[15] * m.mat[3];

	// Column 2
	prod.mat[4]  = mat[0] * m.mat[4] + mat[4] * m.mat[5] + mat[8] * m.mat[6] + mat[12] * m.mat[7];
	prod.mat[5]  = mat[1] * m.mat[4] + mat[5] * m.mat[5] + mat[9] * m.mat[6] + mat[13] * m.mat[7];
	prod.mat[6]  = mat[2] * m.mat[4] + mat[6] * m.mat[5] + mat[10] * m.mat[6] + mat[14] * m.mat[7];
	prod.mat[7]  = mat[3] * m.mat[4] + mat[7] * m.mat[5] + mat[11] * m.mat[6] + mat[15] * m.mat[7];

	// Column 3
	prod.mat[8]  = mat[0] * m.mat[8] + mat[4] * m.mat[9] + mat[8] * m.mat[10] + mat[12] * m.mat[11];
	prod.mat[9]  = mat[1] * m.mat[8] + mat[5] * m.mat[9] + mat[9] * m.mat[10] + mat[13] * m.mat[11];
	prod.mat[10] = mat[2] * m.mat[8] + mat[6] * m.mat[9] + mat[10] * m.mat[10] + mat[14] * m.mat[11];
	prod.mat[11] = mat[3] * m.mat[8] + mat[7] * m.mat[9] + mat[11] * m.mat[10] + mat[15] * m.mat[11];

	// Column 4
	prod.mat[12] = mat[0] * m.mat[12] + mat[4] * m.mat[13] + mat[8] * m.mat[14] + mat[12] * m.mat[15];
	prod.mat[13] = mat[1] * m.mat[12] + mat[5] * m.mat[13] + mat[9] * m.mat[14] + mat[13] * m.mat[15];
	prod.mat[14] = mat[2] * m.mat[12] + mat[6] * m.mat[13] + mat[10] * m.mat[14] + mat[14] * m.mat[15];
	prod.mat[15] = mat[3] * m.mat[12] + mat[7] * m.mat[13] + mat[11] * m.mat[14] + mat[15] * m.mat[15];

	return prod;
}

PI_Vec3 PI_Mat44::operator*(const PI_Vec3 &v) const
{
	return PI_Vec3(mat[0] * v.x + mat[4] * v.y + mat[8] * v.z + mat[12],
				   mat[1] * v.x + mat[5] * v.y + mat[9] * v.z + mat[13],
				   mat[2] * v.x + mat[6] * v.y + mat[10] * v.z + mat[14]);
}
PI_Mat44 &PI_Mat44::operator*=(const PI_Mat44 &m)
{
	return (*this = *this * m);
}

void PI_Plane3::Normalize(void)
{
	float mag = normal.Magnitude();
	normal /= mag;
	d /= mag;
}

float PI_Plane3::DistanceToPoint(const PI_Vec3 &p) const
{
	return normal.Dot(p) + d;
}

bool PI_Ray3::IntersectsPlane(const PI_Plane3 &p, PI_Vec3 &where) const
{
	/*float planeDotDir, t;
	PI_Plane3 dirPlane(dir, 0);
	if (0 == (planeDotDir = p.Dot(dirPlane)))
		return false;

	t = -(p.Dot(end) / planeDotDir);
	where = end + dir * t;
	return true;*/

	float nDotQplusD = p.normal.Dot(end) + p.d, nDotV = p.normal.Dot(dir);
	if (nDotV == 0 && nDotQplusD != 0)
		return false;

	// Compute the point of intersection.
	float t = (-nDotQplusD) / nDotV;
	where = end + dir * t;
	return true;
}

bool PI_Ray3::IntersectsTriangle(const PI_Triangle &tri, PI_Vec3 &where) const
{
	// Construct a plane from the triangle.
	PI_Vec3 faceNorm = (tri.verts[1] - tri.verts[0]).Cross(tri.verts[2] - tri.verts[0]);
	PI_Plane3 triPlane(faceNorm, -faceNorm.Dot(tri.verts[0]));
	
	// First check if the ray intersects the triangle's plane,
	// and if so, where?
	PI_Vec3 isect3D;
	if (!IntersectsPlane(triPlane, isect3D))
		return false;

	// Make this a 2D problem by projecting the triangle onto whatever cardinal plane
	// it is closest to, in terms of parallelism.
	static const unsigned char YZplane = 0, XZplane = 1, XYplane = 2;

	// Which way's the normal pointing "the most" in?
	map<float, unsigned char, greater<float> > twoDmap;
	twoDmap.insert(pair<float, unsigned char>(fabs(faceNorm.x), YZplane));
	twoDmap.insert(pair<float, unsigned char>(fabs(faceNorm.y), XZplane));
	twoDmap.insert(pair<float, unsigned char>(fabs(faceNorm.z), XYplane));

	// Project 3D -> 2D.
	PI_Vec2 p[3], isect2D;
	switch(twoDmap.begin()->second)
	{
		case YZplane:
			isect2D.Set(isect3D.y, isect3D.z);
			p[0].Set(tri.verts[0].y, tri.verts[0].z);
			p[1].Set(tri.verts[1].y, tri.verts[1].z);
			p[2].Set(tri.verts[2].y, tri.verts[2].z);
			break;
		case XZplane:	
			isect2D.Set(isect3D.x, isect3D.z);
			p[0].Set(tri.verts[0].x, tri.verts[0].z);
			p[1].Set(tri.verts[1].x, tri.verts[1].z);
			p[2].Set(tri.verts[2].x, tri.verts[2].z);
			break;
		case XYplane:
			isect2D.Set(isect3D.x, isect3D.y);
			p[0].Set(tri.verts[0].x, tri.verts[0].y);
			p[1].Set(tri.verts[1].x, tri.verts[1].y);
			p[2].Set(tri.verts[2].x, tri.verts[2].y);
			break;
	};

	// Get some edge vectors, which will be rotated 90 degrees to form normals.
	PI_Vec2 norms[3] =
	{
		p[0] - p[1],
		p[1] - p[2],
		p[2] - p[0],
	};
	norms[0].Rotate90DegreesCCW();
	norms[1].Rotate90DegreesCCW();
	norms[2].Rotate90DegreesCCW();

	// All the dot products of edge normals and intersection-corners should be negative if the point's inside.
	if (norms[0].Dot(isect2D - p[0]) >= 0 && norms[1].Dot(isect2D - p[1]) >= 0 && norms[2].Dot(isect2D - p[2]) >= 0)
	{
		where = isect3D;
		return true;
	}

	return false;
}