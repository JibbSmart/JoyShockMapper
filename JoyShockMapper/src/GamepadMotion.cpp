#include "GamepadMotion.h"

#define _USE_MATH_DEFINES
#include <math.h>

namespace GamepadMotionHelpers
{
Quat::Quat()
{
	w = 1.0f;
	x = 0.0f;
	y = 0.0f;
	z = 0.0f;
}

Quat::Quat(float inW, float inX, float inY, float inZ)
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

void Quat::Set(float inW, float inX, float inY, float inZ)
{
	w = inW;
	x = inX;
	y = inY;
	z = inZ;
}

Quat& Quat::operator*=(const Quat& rhs)
{
	Set(w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z,
	  w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y,
	  w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x,
	  w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w);
	return *this;
}

Quat operator*(Quat lhs, const Quat& rhs)
{
	lhs *= rhs;
	return lhs;
}

void Quat::Normalize()
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

Quat Quat::Normalized() const
{
	Quat result = *this;
	result.Normalize();
	return result;
}

void Quat::Invert()
{
	x = -x;
	y = -y;
	z = -z;
	return;
}

Quat Quat::Inverse() const
{
	Quat result = *this;
	result.Invert();
	return result;
}

Vec::Vec()
{
	x = 0.0f;
	y = 0.0f;
	z = 0.0f;
}

Vec::Vec(float inX, float inY, float inZ)
{
	x = inX;
	y = inY;
	z = inZ;
}

void Vec::Set(float inX, float inY, float inZ)
{
	x = inX;
	y = inY;
	z = inZ;
}

float Vec::Length() const
{
	return sqrtf(x * x + y * y + z * z);
}

void Vec::Normalize()
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

Vec Vec::Normalized() const
{
	Vec result = *this;
	result.Normalize();
	return result;
}

Vec& Vec::operator+=(const Vec& rhs)
{
	Set(x + rhs.x, y + rhs.y, z + rhs.z);
	return *this;
}

Vec operator+(Vec lhs, const Vec& rhs)
{
	lhs += rhs;
	return lhs;
}

Vec& Vec::operator-=(const Vec& rhs)
{
	Set(x - rhs.x, y - rhs.y, z - rhs.z);
	return *this;
}

Vec operator-(Vec lhs, const Vec& rhs)
{
	lhs -= rhs;
	return lhs;
}

Vec& Vec::operator*=(const float rhs)
{
	Set(x * rhs, y * rhs, z * rhs);
	return *this;
}

Vec operator*(Vec lhs, const float rhs)
{
	lhs *= rhs;
	return lhs;
}

Vec& Vec::operator/=(const float rhs)
{
	Set(x / rhs, y / rhs, z / rhs);
	return *this;
}

Vec operator/(Vec lhs, const float rhs)
{
	lhs /= rhs;
	return lhs;
}

Vec& Vec::operator*=(const Quat& rhs)
{
	Quat temp = rhs * Quat(0.0f, x, y, z) * rhs.Inverse();
	Set(temp.x, temp.y, temp.z);
	return *this;
}

Vec operator*(Vec lhs, const Quat& rhs)
{
	lhs *= rhs;
	return lhs;
}

Vec Vec::operator-() const
{
	Vec result = Vec(-x, -y, -z);
	return result;
}

float Vec::Dot(const Vec& other) const
{
	return x * other.x + y * other.y + z * other.z;
}

Vec Vec::Cross(const Vec& other) const
{
	return Vec(y * other.z - z * other.y,
	  z * other.x - x * other.z,
	  x * other.y - y * other.x);
}

Motion::Motion()
{
	Reset();
}

void Motion::Reset()
{
	Quaternion.Set(1.0f, 0.0f, 0.0f, 0.0f);
	Accel.Set(0.0f, 0.0f, 0.0f);
	Grav.Set(0.0f, 0.0f, 0.0f);
	NumGravDirectionSamplesCounted = 0;
}

/// <summary>
/// The gyro inputs should be calibrated degrees per second but have no other processing. Acceleration is in G units (1 = approx. 9.8m/s^2)
/// </summary>
void Motion::Update(float inGyroX, float inGyroY, float inGyroZ, float inAccelX, float inAccelY, float inAccelZ, float gravityLength, float deltaTime)
{
	const Vec axis = Vec(inGyroX, inGyroY, inGyroZ);
	const Vec accel = Vec(inAccelX, inAccelY, inAccelZ);
	float angle = axis.Length() * (float)M_PI / 180.0f;
	angle *= deltaTime;

	// rotate
	Quat rotation = AngleAxis(angle, axis.x, axis.y, axis.z);
	Quaternion *= rotation; // do it this way because it's a local rotation, not global
	//printf("Quat: %.4f %.4f %.4f %.4f _",
	//	Quaternion.w, Quaternion.x, Quaternion.y, Quaternion.z);
	float accelMagnitude = accel.Length();
	if (accelMagnitude > 0.0f)
	{
		const Vec accelNorm = accel / accelMagnitude;
		LastGravityIdx = (LastGravityIdx + NumGravDirectionSamples - 1) % NumGravDirectionSamples;
		// for comparing and perhaps smoothing gravity samples, we need them to be global
		Vec absoluteAccel = accel * Quaternion;
		GravDirectionSamples[LastGravityIdx] = absoluteAccel;
		Vec gravityMin = absoluteAccel;
		Vec gravityMax = absoluteAccel;
		const float steadyGravityThreshold = 0.05f;
		NumGravDirectionSamplesCounted++;
		const int numGravSamples = NumGravDirectionSamplesCounted < NumGravDirectionSamples ? NumGravDirectionSamplesCounted : NumGravDirectionSamples;
		for (int idx = 1; idx < numGravSamples; idx++)
		{
			Vec thisSample = GravDirectionSamples[(LastGravityIdx + idx) % NumGravDirectionSamples];
			if (thisSample.x > gravityMax.x)
			{
				gravityMax.x = thisSample.x;
			}
			if (thisSample.y > gravityMax.y)
			{
				gravityMax.y = thisSample.y;
			}
			if (thisSample.z > gravityMax.z)
			{
				gravityMax.z = thisSample.z;
			}
			if (thisSample.x < gravityMin.x)
			{
				gravityMin.x = thisSample.x;
			}
			if (thisSample.y < gravityMin.y)
			{
				gravityMin.y = thisSample.y;
			}
			if (thisSample.y < gravityMin.y)
			{
				gravityMin.z = thisSample.z;
			}
		}
		const Vec gravityBoxSize = gravityMax - gravityMin;
		//printf(" Gravity Box Size: %.4f _ ", gravityBoxSize.Length());
		if (gravityBoxSize.x <= steadyGravityThreshold &&
		  gravityBoxSize.y <= steadyGravityThreshold &&
		  gravityBoxSize.z <= steadyGravityThreshold)
		{
			absoluteAccel = gravityMin + (gravityBoxSize * 0.5f);
			const Vec gravityDirection = -absoluteAccel.Normalized();
			const Vec expectedGravity = Vec(0.0f, -1.0f, 0.0f) * Quaternion.Inverse();
			const float errorAngle = acosf(Vec(0.0f, -1.0f, 0.0f).Dot(gravityDirection)) * 180.0f / (float)M_PI;

			const Vec flattened = gravityDirection.Cross(Vec(0.0f, -1.0f, 0.0f)).Normalized();

			if (errorAngle > 0.0f)
			{
				const float EaseInTime = 0.25f;
				TimeCorrecting += deltaTime;

				const float tighteningThreshold = 5.0f;

				float confidentSmoothCorrect = errorAngle;
				confidentSmoothCorrect *= 1.0f - exp2f(-deltaTime * 4.0f);

				if (TimeCorrecting < EaseInTime)
				{
					confidentSmoothCorrect *= TimeCorrecting / EaseInTime;
				}

				Quaternion = AngleAxis(confidentSmoothCorrect * (float)M_PI / 180.0f, flattened.x, flattened.y, flattened.z) * Quaternion;
			}
			else
			{
				TimeCorrecting = 0.0f;
			}

			Grav = Vec(0.0f, -gravityLength, 0.0f) * Quaternion.Inverse();
			Accel = accel + Grav; // gravity won't be shaky. accel might. so let's keep using the quaternion's calculated gravity vector.
		}
		else
		{
			TimeCorrecting = 0.0f;
			Grav = Vec(0.0f, -gravityLength, 0.0f) * Quaternion.Inverse();
			Accel = accel + Grav;
		}
	}
	else
	{
		TimeCorrecting = 0.0f;
		Accel.Set(0.0f, 0.0f, 0.0f);
	}
	Quaternion.Normalize();
}
} // namespace GamepadMotionHelpers

GamepadMotion::GamepadMotion()
{
	Reset();
}

void GamepadMotion::Reset()
{
	GyroCalibration = {};
	Gyro = {};
	RawAccel = {};
	Motion.Reset();
}

void GamepadMotion::ProcessMotion(float gyroX, float gyroY, float gyroZ,
  float accelX, float accelY, float accelZ, float deltaTime)
{
	float accelMagnitude = sqrtf(accelX * accelX + accelY * accelY + accelZ * accelZ);

	if (IsCalibrating)
	{
		PushSensorSamples(gyroX, gyroY, gyroZ, accelMagnitude);
	}

	float gyroOffsetX, gyroOffsetY, gyroOffsetZ;
	GetCalibratedSensor(gyroOffsetX, gyroOffsetY, gyroOffsetZ, accelMagnitude);

	gyroX -= gyroOffsetX;
	gyroY -= gyroOffsetY;
	gyroZ -= gyroOffsetZ;

	Motion.Update(gyroX, gyroY, gyroZ, accelX, accelY, accelZ, accelMagnitude, deltaTime);

	Gyro.x = gyroX;
	Gyro.y = gyroY;
	Gyro.z = gyroZ;
	RawAccel.x = accelX;
	RawAccel.y = accelY;
	RawAccel.z = accelZ;
}

// reading the current state
void GamepadMotion::GetCalibratedGyro(float& x, float& y, float& z)
{
	x = Gyro.x;
	y = Gyro.y;
	z = Gyro.z;
}

void GamepadMotion::GetGravity(float& x, float& y, float& z)
{
	x = Motion.Grav.x;
	y = Motion.Grav.y;
	z = Motion.Grav.z;
}

void GamepadMotion::GetProcessedAcceleration(float& x, float& y, float& z)
{
	x = Motion.Accel.x;
	y = Motion.Accel.y;
	z = Motion.Accel.z;
}

void GamepadMotion::GetOrientation(float& w, float& x, float& y, float& z)
{
	w = Motion.Quaternion.w;
	x = Motion.Quaternion.x;
	y = Motion.Quaternion.y;
	z = Motion.Quaternion.z;
}

// gyro calibration functions
void GamepadMotion::StartContinuousCalibration()
{
	IsCalibrating = true;
}

void GamepadMotion::PauseContinuousCalibration()
{
	IsCalibrating = false;
}

void GamepadMotion::ResetContinuousCalibration()
{
	GyroCalibration = {};
}

void GamepadMotion::GetCalibrationOffset(float& xOffset, float& yOffset, float& zOffset)
{
	float accelMagnitude;
	GetCalibratedSensor(xOffset, yOffset, zOffset, accelMagnitude);
}

void GamepadMotion::SetCalibrationOffset(float xOffset, float yOffset, float zOffset, int weight)
{
	if (GyroCalibration.NumSamples > 1)
	{
		GyroCalibration.AccelMagnitude *= ((float)weight) / GyroCalibration.NumSamples;
	}
	else
	{
		GyroCalibration.AccelMagnitude = weight;
	}

	GyroCalibration.NumSamples = weight;
	GyroCalibration.X = xOffset * weight;
	GyroCalibration.Y = yOffset * weight;
	GyroCalibration.Z = zOffset * weight;
}

void GamepadMotion::ResetMotion()
{
	Motion.Reset();
}

// Private Methods

void GamepadMotion::PushSensorSamples(float gyroX, float gyroY, float gyroZ, float accelMagnitude)
{
	// accumulate
	GyroCalibration.NumSamples++;
	GyroCalibration.X += gyroX;
	GyroCalibration.Y += gyroY;
	GyroCalibration.Z += gyroZ;
	GyroCalibration.AccelMagnitude += accelMagnitude;
}

void GamepadMotion::GetCalibratedSensor(float& gyroOffsetX, float& gyroOffsetY, float& gyroOffsetZ, float& accelMagnitude)
{
	if (GyroCalibration.NumSamples <= 0)
	{
		gyroOffsetX = 0.f;
		gyroOffsetY = 0.f;
		gyroOffsetZ = 0.f;
		accelMagnitude = 0.f;
		return;
	}

	float inverseSamples = 1.f / GyroCalibration.NumSamples;
	gyroOffsetX = GyroCalibration.X * inverseSamples;
	gyroOffsetY = GyroCalibration.Y * inverseSamples;
	gyroOffsetZ = GyroCalibration.Z * inverseSamples;
	accelMagnitude = GyroCalibration.AccelMagnitude * inverseSamples;
}
