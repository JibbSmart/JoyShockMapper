#define _USE_MATH_DEFINES
#include <math.h>

struct Quat
{
	float w;
	float x;
	float y;
	float z;

	Quat()
	{
		w = 1.0f;
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
	}

	Quat(float inW, float inX, float inY, float inZ)
	{
		w = inW;
		x = inX;
		y = inY;
		z = inZ;
	}

	static Quat AngleAxis(float inAngle, float inX, float inY, float inZ)
	{
		Quat result = Quat(cosf(inAngle * 0.5f), inX, inY, inZ);
		result.Normalize();
		return result;
	}

	void Set(float inW, float inX, float inY, float inZ)
	{
		w = inW;
		x = inX;
		y = inY;
		z = inZ;
	}

	Quat& operator*=(const Quat& rhs)
	{
		Set(w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z,
		  w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y,
		  w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x,
		  w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w);
		return *this;
	}

	friend Quat operator*(Quat lhs, const Quat& rhs)
	{
		lhs *= rhs;
		return lhs;
	}

	void Normalize()
	{
		//printf("Normalizing: %.4f, %.4f, %.4f, %.4f\n", w, x, y, z);
		const float length = sqrtf(x * x + y * y + z * z);
		float targetLength = 1.0f - w * w;
		if (targetLength <= 0.0f || length <= 0.0f)
		{
			Set(1.0f, 0.0f, 0.0f, 0.0f);
			return;
		}
		targetLength = sqrtf(targetLength);
		const float fixFactor = targetLength / length;

		x *= fixFactor;
		y *= fixFactor;
		z *= fixFactor;

		//printf("Normalized: %.4f, %.4f, %.4f, %.4f\n", w, x, y, z);
		return;
	}

	Quat Normalized() const
	{
		Quat result = *this;
		result.Normalize();
		return result;
	}

	void Invert()
	{
		x = -x;
		y = -y;
		z = -z;
		return;
	}

	Quat Inverse() const
	{
		Quat result = *this;
		result.Invert();
		return result;
	}
};

struct Vec
{
	float x;
	float y;
	float z;

	Vec()
	{
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
	}

	Vec(float inX, float inY, float inZ)
	{
		x = inX;
		y = inY;
		z = inZ;
	}

	void Set(float inX, float inY, float inZ)
	{
		x = inX;
		y = inY;
		z = inZ;
	}

	float Length() const
	{
		return sqrtf(x * x + y * y + z * z);
	}

	void Normalize()
	{
		const float length = Length();
		if (length == 0.0)
		{
			return;
		}
		const float fixFactor = 1.0f / length;

		x *= fixFactor;
		y *= fixFactor;
		z *= fixFactor;
		return;
	}

	Vec Normalized() const
	{
		Vec result = *this;
		result.Normalize();
		return result;
	}

	Vec& operator+=(const Vec& rhs)
	{
		Set(x + rhs.x, y + rhs.y, z + rhs.z);
		return *this;
	}

	friend Vec operator+(Vec lhs, const Vec& rhs)
	{
		lhs += rhs;
		return lhs;
	}

	Vec& operator-=(const Vec& rhs)
	{
		Set(x - rhs.x, y - rhs.y, z - rhs.z);
		return *this;
	}

	friend Vec operator-(Vec lhs, const Vec& rhs)
	{
		lhs -= rhs;
		return lhs;
	}

	Vec& operator*=(const float rhs)
	{
		Set(x * rhs, y * rhs, z * rhs);
		return *this;
	}

	friend Vec operator*(Vec lhs, const float rhs)
	{
		lhs *= rhs;
		return lhs;
	}

	Vec& operator/=(const float rhs)
	{
		Set(x / rhs, y / rhs, z / rhs);
		return *this;
	}

	friend Vec operator/(Vec lhs, const float rhs)
	{
		lhs /= rhs;
		return lhs;
	}

	Vec& operator*=(const Quat& rhs)
	{
		Quat temp = rhs * Quat(0.0f, x, y, z) * rhs.Inverse();
		Set(temp.x, temp.y, temp.z);
		return *this;
	}

	friend Vec operator*(Vec lhs, const Quat& rhs)
	{
		lhs *= rhs;
		return lhs;
	}

	Vec operator-() const
	{
		Vec result = Vec(-x, -y, -z);
		return result;
	}

	float Dot(const Vec& other) const
	{
		return x * other.x + y * other.y + z * other.z;
	}

	Vec Cross(const Vec& other) const
	{
		return Vec(y * other.z - z * other.y,
		  z * other.x - x * other.z,
		  x * other.y - y * other.x);
	}
};