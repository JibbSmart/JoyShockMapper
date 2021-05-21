// Copyright (c) 2020-2021 Julian "Jibb" Smart
// Released under the MIT license. See https://github.com/JibbSmart/GamepadMotionHelpers/blob/main/LICENSE for more info
// Revision 3

#pragma once

// You don't need to look at these. These will just be used internally by the GamepadMotion class declared below.
// You can ignore anything in namespace GamepadMotionHelpers.

namespace GamepadMotionHelpers
{
struct GyroCalibration
{
	float X;
	float Y;
	float Z;
	float AccelMagnitude;
	int NumSamples;
};

struct Quat
{
	float w;
	float x;
	float y;
	float z;

	Quat();
	Quat(float inW, float inX, float inY, float inZ);
	void Set(float inW, float inX, float inY, float inZ);
	Quat& operator*=(const Quat& rhs);
	friend Quat operator*(Quat lhs, const Quat& rhs);
	void Normalize();
	Quat Normalized() const;
	void Invert();
	Quat Inverse() const;
};

struct Vec
{
	float x;
	float y;
	float z;

	Vec();
	Vec(float inX, float inY, float inZ);
	void Set(float inX, float inY, float inZ);
	float Length() const;
	void Normalize();
	Vec Normalized() const;
	float Dot(const Vec& other) const;
	Vec Cross(const Vec& other) const;
	Vec& operator+=(const Vec& rhs);
	friend Vec operator+(Vec lhs, const Vec& rhs);
	Vec& operator-=(const Vec& rhs);
	friend Vec operator-(Vec lhs, const Vec& rhs);
	Vec& operator*=(const float rhs);
	friend Vec operator*(Vec lhs, const float rhs);
	Vec& operator/=(const float rhs);
	friend Vec operator/(Vec lhs, const float rhs);
	Vec& operator*=(const Quat& rhs);
	friend Vec operator*(Vec lhs, const Quat& rhs);
	Vec operator-() const;
};

struct Motion
{
	Quat Quaternion;
	Vec Accel;
	Vec Grav;

	const int NumGravDirectionSamples = 10;
	Vec GravDirectionSamples[10];
	int LastGravityIdx = 9;
	int NumGravDirectionSamplesCounted = 0;
	float TimeCorrecting = 0.0f;

	Motion();
	void Reset();
	void Update(float inGyroX, float inGyroY, float inGyroZ, float inAccelX, float inAccelY, float inAccelZ, float gravityLength, float deltaTime);
};
} // namespace GamepadMotionHelpers

// Note that I'm using a Y-up coordinate system. This is to follow the convention set by the motion sensors in
// PlayStation controllers, which was what I was using when writing in this. But for the record, Z-up is
// better for most games (XY ground-plane in 3D games simplifies using 2D vectors in navigation, for example).

// Gyro units should be degrees per second. Accelerometer should be Gs (approx. 9.8m/s^2 = 1G). If you're using
// radians per second, meters per second squared, etc, conversion should be simple.

class GamepadMotion
{
public:
	GamepadMotion();

	void Reset();

	void ProcessMotion(float gyroX, float gyroY, float gyroZ,
	  float accelX, float accelY, float accelZ, float deltaTime);

	// reading the current state
	void GetCalibratedGyro(float& x, float& y, float& z);
	void GetGravity(float& x, float& y, float& z);
	void GetProcessedAcceleration(float& x, float& y, float& z);
	void GetOrientation(float& w, float& x, float& y, float& z);

	// gyro calibration functions
	void StartContinuousCalibration();
	void PauseContinuousCalibration();
	void ResetContinuousCalibration();
	void GetCalibrationOffset(float& xOffset, float& yOffset, float& zOffset);
	void SetCalibrationOffset(float xOffset, float yOffset, float zOffset, int weight);

	void ResetMotion();

private:
	GamepadMotionHelpers::Vec Gyro;
	GamepadMotionHelpers::Vec RawAccel;
	GamepadMotionHelpers::Motion Motion;
	GamepadMotionHelpers::GyroCalibration GyroCalibration;

	bool IsCalibrating;
	void PushSensorSamples(float gyroX, float gyroY, float gyroZ, float accelMagnitude);
	void GetCalibratedSensor(float& gyroOffsetX, float& gyroOffsetY, float& gyroOffsetZ, float& accelMagnitude);
};
