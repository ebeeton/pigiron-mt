// PigIron math interfaces.
//
// Copyright Evan Beeton 2/1/2005

#pragma once

class PI_Mat44;

class PI_Triangle;

// A 2D Vector
class PI_Vec2
{
	public:
		float x, y;

		PI_Vec2(float _x = 0, float _y = 0) : x(_x), y(_y) { }

		void Set(float _x, float _y)
		{
			x = _x, y = _y;	
		}

		PI_Vec2 operator-(const PI_Vec2 &r) const
		{
			return PI_Vec2(x - r.x, y - r.y);
		}

		PI_Vec2 &operator*=(float s)
		{
			x *= s, y *= s;
			return *this;
		}

		float Dot(const PI_Vec2 &r) const
		{
			return x * r.x + y * r.y;
		}

		void Rotate90DegreesCCW(void)
		{
			float temp = x;
			x = -y;
			y = temp;
		}

		float magnitude(void) const;

		void normalize(void);
};

// A 3D Vector
class PI_Vec3
{
	bool operator<(const PI_Vec3 &rhs) const;
	bool operator<=(const PI_Vec3 &rhs) const;
	bool operator>=(const PI_Vec3 &rhs) const;
	bool operator>(const PI_Vec3 &rhs) const;
    
public:
	
	float x, y, z;

	explicit PI_Vec3(float _x = 0, float _y = 0, float _z = 0) : x(_x), y(_y), z(_z) { }

	void Set(float _x, float _y, float _z)
	{
		x = _x, y = _y, z = _z;
	}

	void ZeroOut(void)
	{
		x = y = z = 0;
	}

	PI_Vec3 operator+(const PI_Vec3 &rhs) const
	{
		return PI_Vec3(x + rhs.x, y + rhs.y, z + rhs.z);
	}

	PI_Vec3 &operator+=(const PI_Vec3 &rhs)
	{
		x += rhs.x, y += rhs.y, z += rhs.z;
		return *this;
	}

	PI_Vec3 operator-(void) const
	{
		return PI_Vec3(-x, -y, -z);
	}

	PI_Vec3 operator-(const PI_Vec3 &rhs) const
	{
		return PI_Vec3(x - rhs.x, y - rhs.y, z - rhs.z);
	}

	PI_Vec3 &operator-=(const PI_Vec3 &rhs)
	{
		x -= rhs.x, y -= rhs.y, z -= rhs.z;
		return *this;
	}

	PI_Vec3 operator*(float f) const
	{
		return PI_Vec3(x * f, y * f, z * f);
	}

	PI_Vec3 &operator*=(float f)
	{
		x *= f, y *= f, z *= f;
		return *this;
	}

	PI_Vec3 operator/(float f) const
	{
		return PI_Vec3(x / f, y / f, z / f);
	}

	PI_Vec3 &operator/=(float f)
	{
		x /= f, y /= f, z /= f;
		return *this;
	}

	float Dot(const PI_Vec3 &rhs) const
	{
		return x * rhs.x + y * rhs.y + z * rhs.z;
	}

	PI_Vec3 Cross(const PI_Vec3 &rhs) const {
		return PI_Vec3(y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x);
	}

	// time is 0 -> 1
	PI_Vec3 Lerp(const PI_Vec3 &rhs, float time) 
	{
		float omt = 1 - time;
		return PI_Vec3(omt * x + time * rhs.x,
					   omt * y + time * rhs.y,
					   omt * z + time * rhs.z);
	}

	bool operator==(const PI_Vec3 &rhs) const
	{
		return x == rhs.x && y == rhs.y && z == rhs.z;
	}

	bool operator!=(const PI_Vec3 &rhs) const
	{
		return x != rhs.x || y != rhs.y || z != rhs.z;
	}

	operator const float *(void) const { return &x; }

	float MagnitudeSquared(void) const
	{
		return x * x + y * y + z * z;
	}

	float Magnitude(void) const;

	void Normalize(void);

	// Rotate this vector by the transposed rotational components of m.
	PI_Vec3 &TransposedRotate(const PI_Mat44 &m);

	// Rotate this vector by the rotational components of m.
	PI_Vec3 &Rotate(const PI_Mat44 &m);
};

// A 4x4 matrix with functionality for 3D transforms.
//
//  row-column/subscript
//
//	11/0	12/4	13/8	14/12
//	21/1	22/5	23/9	24/13
//	31/2	32/6	33/10	34/14
//	41/3	42/7	43/11	44/15
//
class PI_Mat44
{
	// Prevent the conversion functions from allowing you compare matricies.
	bool operator==(const PI_Mat44 &rhs) const;
	bool operator!=(const PI_Mat44 &rhs) const;
	bool operator<(const PI_Mat44 &rhs) const;
	bool operator<=(const PI_Mat44 &rhs) const;
	bool operator>=(const PI_Mat44 &rhs) const;
	bool operator>(const PI_Mat44 &rhs) const;

public:
	// The column-major components.
	float mat[16];

	PI_Mat44(bool initIdentity = true)
	{
		if (initIdentity)
			SetIdentity();
	}

	void SetIdentity(void)
	{
		// Is this faster than memset?
		mat[1] = mat[2] = mat[3] = mat[4] = mat[6] = mat[7] = mat[8] = mat[9] = mat[11] = mat[12] = mat[13] = mat[14] = mat[15] = 0;
		mat[0] = mat[5] = mat[10] = mat[15] = 1;
	}

	void SetTranslation(const PI_Vec3 &where)
	{
		mat[12] = where.x;
		mat[13] = where.y;
		mat[14] = where.z;
	}

	void Translate(const PI_Vec3 &offset)
	{
		mat[12] += offset.x;
		mat[13] += offset.y;
		mat[14] += offset.z;
	}

	void TranslateLocalZ(float s)
	{
		float x = mat[8] * s, y = mat[9] * s, z = mat[10] * s;
		mat[12] += x;
		mat[13] += y;
		mat[14] += z;
	}

	void SetAxisX(const PI_Vec3 &v)
	{
		mat[0] = v.x, mat[1] = v.y, mat[2] = v.z;
	}

	void SetAxisY(const PI_Vec3 &v)
	{
		mat[4] = v.x, mat[5] = v.y, mat[6] = v.z;
	}

	void SetAxisZ(const PI_Vec3 &v)
	{
		mat[8] = v.x, mat[9] = v.y, mat[10] = v.z;
	}

	PI_Vec3 GetAxisX(void) const
	{
		return PI_Vec3(mat[0], mat[1], mat[2]);
	}

	PI_Vec3 GetAxisY(void) const
	{
		return PI_Vec3(mat[4], mat[5], mat[6]);
	}

	PI_Vec3 GetAxisZ(void) const
	{
		return PI_Vec3(mat[8], mat[9], mat[10]);
	}

	PI_Vec3 GetTranslation(void) const
	{
		return PI_Vec3(mat[12], mat[13], mat[14]);
	}

	// Conversion functions for accessing the components directly.
	operator const float*(void) const { return mat; }
	operator float*(void) { return mat; }

	// Constructor for building a rotation matrix around an arbitrary axis.
	PI_Mat44(const PI_Vec3 &axis, float angleRadians);

	// This function only manipulates the 3x3 rotational components of the matrix.
	void MakeRotationMatrix(const PI_Vec3 &axis, float angleR);

	// Copy the rotation components of another matrix into this one.
	PI_Mat44 &CopyRotation(const PI_Mat44 &m);

	PI_Mat44 operator*(const PI_Mat44 &m) const;
	PI_Mat44 &operator*=(const PI_Mat44 &m);
	PI_Vec3 operator*(const PI_Vec3 &v) const;
};

// Left && Right == YZ
// Bottom && Top == XZ
// Near && Far == XY
enum Planes { PlaneLeft, PlaneRight, PlaneBottom, PlaneTop, PlaneNear, PlaneFar };

// A 3D Plane
class PI_Plane3
{
	public:
		PI_Vec3 normal;
		float d;

		PI_Plane3(const PI_Vec3 &norm, float D) : normal(norm), d(D) { }

		PI_Plane3(void) : d(0) { }

		void Set(const PI_Vec3 &norm, float D)
		{
			normal = norm;
			d = D;
		}

		float Dot(const PI_Plane3 &p) const
		{
			return normal.Dot(p.normal) + d * p.d;
		}

		float DotHomogenous(const PI_Vec3 &v) const
		{
			// The vector has an implicit w component of 1.
			return normal.Dot(v) + d;
		}

		PI_Plane3 operator-(void) const
		{
			return PI_Plane3(-normal, -d);
		}

		void Normalize(void);

		float DistanceToPoint(const PI_Vec3 &p) const;


};

// A 3D Ray
class PI_Ray3
{
	public:
		PI_Vec3 end, dir;

		PI_Ray3(void) { }

		PI_Ray3(const PI_Vec3 &_end, const PI_Vec3 &_dir)
		{
			end = _end, dir = _dir;
		}

		bool IntersectsPlane(const PI_Plane3 &p, PI_Vec3 &where) const;

		bool IntersectsTriangle(const PI_Triangle &tri, PI_Vec3 &where) const;
};