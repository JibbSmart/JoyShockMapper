// Copyright (c) 2020-2021 Julian "Jibb" Smart
// Released under the MIT license. See https://github.com/JibbSmart/GamepadMotionHelpers/blob/main/LICENSE for more info
// Revision 5

#define _USE_MATH_DEFINES
#include <math.h>

// You don't need to look at these. These will just be used internally by the GamepadMotion class declared below.
// You can ignore anything in namespace GamepadMotionHelpers.
class GamepadMotionSettings;
class GamepadMotion;

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
		Vec(float inValue);
		Vec(float inX, float inY, float inZ);
		void Set(float inX, float inY, float inZ);
		float Length() const;
		float LengthSquared() const;
		void Normalize();
		Vec Normalized() const;
		float Dot(const Vec& other) const;
		Vec Cross(const Vec& other) const;
		Vec Min(const Vec& other) const;
		Vec Max(const Vec& other) const;
		Vec Abs() const;
		Vec Lerp(const Vec& other, float factor) const;
		Vec Lerp(const Vec& other, const Vec& factor) const;
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

	struct SensorMinMaxWindow
	{
		Vec MinGyro;
		Vec MaxGyro;
		Vec MeanGyro;
		Vec MinAccel;
		Vec MaxAccel;
		Vec MeanAccel;
		Vec StartAccel;
		int NumSamples;
		float TimeSampled;

		SensorMinMaxWindow();
		void Reset(float remainder);
		void AddSample(const Vec& inGyro, const Vec& inAccel, float deltaTime);
		Vec GetMidGyro();
	};

	struct AutoCalibration
	{
		SensorMinMaxWindow MinMaxWindow;
		Vec SmoothedAngularVelocityGyro;
		Vec SmoothedAngularVelocityAccel;
		Vec SmoothedPreviousAccel;
		Vec PreviousAccel;

		AutoCalibration();
		bool AddSampleStillness(const Vec& inGyro, const Vec& inAccel, float deltaTime, bool doSensorFusion);
		void NoSampleStillness();
		bool AddSampleSensorFusion(const Vec& inGyro, const Vec& inAccel, float deltaTime);
		void NoSampleSensorFusion();
		void SetCalibrationData(GyroCalibration* calibrationData);
		void SetSettings(GamepadMotionSettings* settings);

	private:
		Vec MinDeltaGyro = Vec(10.f);
		Vec MinDeltaAccel = Vec(10.f);
		float RecalibrateThreshold = 1.f;
		float SensorFusionSkippedTime = 0.f;
		float TimeSteadySensorFusion = 0.f;
		float TimeSteadyStillness = 0.f;

		GyroCalibration* CalibrationData;
		GamepadMotionSettings* Settings;
	};

	struct Motion
	{
		Quat Quaternion;
		Vec Accel;
		Vec Grav;

		Vec ShortSmoothAccel;
		Vec LongSmoothAccel;
		const float ShortSteadinessHalfTime = 0.25f;
		const float LongSteadinessHalfTime = 1.f;

		float TimeCorrecting = 0.f;

		Motion();
		void Reset();
		void Update(float inGyroX, float inGyroY, float inGyroZ, float inAccelX, float inAccelY, float inAccelZ, float gravityLength, float deltaTime);
		void SetSettings(GamepadMotionSettings* settings);

	private:
		GamepadMotionSettings* Settings;
	};

	enum CalibrationMode
	{
		Manual = 0,
		Stillness = 1,
		SensorFusion = 2,
	};
	
	// https://stackoverflow.com/a/1448478/1130520
	inline CalibrationMode operator|(CalibrationMode a, CalibrationMode b)
	{
	    return static_cast<CalibrationMode>(static_cast<int>(a) | static_cast<int>(b));
	}
	
	inline CalibrationMode operator&(CalibrationMode a, CalibrationMode b)
	{
	    return static_cast<CalibrationMode>(static_cast<int>(a) & static_cast<int>(b));
	}
	
	inline CalibrationMode operator~(CalibrationMode a)
	{
		return static_cast<CalibrationMode>(~static_cast<int>(a));
	}
	
	// https://stackoverflow.com/a/23152590/1130520
	inline CalibrationMode& operator|=(CalibrationMode& a, CalibrationMode b)
	{
		return (CalibrationMode&)((int&)(a) |= static_cast<int>(b));
	}
	
	inline CalibrationMode& operator&=(CalibrationMode& a, CalibrationMode b)
	{
		return (CalibrationMode&)((int&)(a) &= static_cast<int>(b));
	}
}

// Note that I'm using a Y-up coordinate system. This is to follow the convention set by the motion sensors in
// PlayStation controllers, which was what I was using when writing in this. But for the record, Z-up is
// better for most games (XY ground-plane in 3D games simplifies using 2D vectors in navigation, for example).

// Gyro units should be degrees per second. Accelerometer should be g-force (approx. 9.8 m/s^2 = 1 g). If you're using
// radians per second, meters per second squared, etc, conversion should be simple.

class GamepadMotionSettings
{
public:
	int MinStillnessSamples = 10;
	float MinStillnessCollectionTime = 0.5f;
	float MinStillnessCorrectionTime = 2.f;
	float MaxStillnessError = 1.25f;
	float StillnessSampleDeteriorationRate = 0.2f;
	float StillnessErrorClimbRate = 0.1f;
	float StillnessErrorDropOnRecalibrate = 0.1f;
	float StillnessCalibrationEaseInTime = 3.f;
	float StillnessCalibrationHalfTime = 0.1f;

	float SensorFusionCalibrationSmoothingStrength = 2.f;
	float SensorFusionAngularAccelerationThreshold = 20.f;
	float SensorFusionCalibrationEaseInTime = 3.f;
	float SensorFusionCalibrationHalfTime = 0.1f;

	float SteadyGravityThreshold = 0.03f;
	float GravityCorrectEaseInTime = 0.25f;
	float GravityCorrectHalfTime = 0.25f;
};

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

	GamepadMotionHelpers::CalibrationMode GetCalibrationMode();
	void SetCalibrationMode(GamepadMotionHelpers::CalibrationMode calibrationMode);

	void ResetMotion();

	GamepadMotionSettings Settings;

private:
	GamepadMotionHelpers::Vec Gyro;
	GamepadMotionHelpers::Vec RawAccel;
	GamepadMotionHelpers::Motion Motion;
	GamepadMotionHelpers::GyroCalibration GyroCalibration;
	GamepadMotionHelpers::AutoCalibration AutoCalibration;
	GamepadMotionHelpers::CalibrationMode CurrentCalibrationMode;

	bool IsCalibrating;
	void PushSensorSamples(float gyroX, float gyroY, float gyroZ, float accelMagnitude);
	void GetCalibratedSensor(float& gyroOffsetX, float& gyroOffsetY, float& gyroOffsetZ, float& accelMagnitude);
};

///////////// Everything below here are just implementation details /////////////

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

	Vec::Vec(float inValue)
	{
		x = inValue;
		y = inValue;
		z = inValue;
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

	float Vec::LengthSquared() const
	{
		return x * x + y * y + z * z;
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

	Vec Vec::Min(const Vec& other) const
	{
		return Vec(x < other.x ? x : other.x,
			y < other.y ? y : other.y,
			z < other.z ? z : other.z);
	}
	
	Vec Vec::Max(const Vec& other) const
	{
		return Vec(x > other.x ? x : other.x,
			y > other.y ? y : other.y,
			z > other.z ? z : other.z);
	}

	Vec Vec::Abs() const
	{
		return Vec(x > 0 ? x : -x,
			y > 0 ? y : -y,
			z > 0 ? z : -z);
	}

	Vec Vec::Lerp(const Vec& other, float factor) const
	{
		return *this + (other - *this) * factor;
	}

	Vec Vec::Lerp(const Vec& other, const Vec& factor) const
	{
		return Vec(this->x + (other.x - this->x) * factor.x,
			this->y + (other.y - this->y) * factor.y,
			this->z + (other.z - this->z) * factor.z);
	}

	Motion::Motion()
	{
		Reset();
	}

	void Motion::Reset()
	{
		Quaternion.Set(1.f, 0.f, 0.f, 0.f);
		Accel.Set(0.f, 0.f, 0.f);
		Grav.Set(0.f, 0.f, 0.f);
		ShortSmoothAccel.Set(0.f, 0.f, 0.f);
		LongSmoothAccel.Set(0.f, 0.f, 0.f);
	}

	/// <summary>
	/// The gyro inputs should be calibrated degrees per second but have no other processing. Acceleration is in G units (1 = approx. 9.8m/s^2)
	/// </summary>
	void Motion::Update(float inGyroX, float inGyroY, float inGyroZ, float inAccelX, float inAccelY, float inAccelZ, float gravityLength, float deltaTime)
	{
		if (!Settings)
		{
			return;
		}

		// get settings
		const float steadyGravityThreshold = Settings->SteadyGravityThreshold;
		const float gravityCorrectEaseInTime = Settings->GravityCorrectEaseInTime;
		const float gravityCorrectHalfTime = Settings->GravityCorrectHalfTime;

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
			// for comparing and smoothing gravity samples, we need them to be global
			Vec absoluteAccel = accel * Quaternion;
			//printf("Absolute Accel: %.4f %.4f %.4f\n",
			//	absoluteAccel.x, absoluteAccel.y, absoluteAccel.z);
			const float shortSmoothFactor = ShortSteadinessHalfTime <= 0.f ? 0.f : exp2f(-deltaTime / ShortSteadinessHalfTime);
			ShortSmoothAccel = absoluteAccel.Lerp(ShortSmoothAccel, shortSmoothFactor);
			const float longSmoothFactor = LongSteadinessHalfTime <= 0.f ? 0.f : exp2f(-deltaTime / LongSteadinessHalfTime);
			LongSmoothAccel = absoluteAccel.Lerp(LongSmoothAccel, longSmoothFactor);
			//printf(" Gravity Box Size: %.4f _ ", gravityBoxSize.Length());
			if ((LongSmoothAccel - ShortSmoothAccel).LengthSquared() <= steadyGravityThreshold*steadyGravityThreshold)
			{
				/*if (TimeCorrecting == 0.f)
				{
					printf("Gravity Steady\n");
				}/**/
				TimeCorrecting += deltaTime;

				absoluteAccel = ShortSmoothAccel;
				const Vec gravityDirection = -absoluteAccel.Normalized();
				const Vec expectedGravity = Vec(0.0f, -1.0f, 0.0f) * Quaternion.Inverse();
				const float errorAngle = acosf(Vec(0.0f, -1.0f, 0.0f).Dot(gravityDirection)) * 180.0f / (float)M_PI;

				const Vec flattened = gravityDirection.Cross(Vec(0.0f, -1.0f, 0.0f)).Normalized();

				if (errorAngle > 0.0f)
				{
					const float correctFactor = gravityCorrectHalfTime <= 0.f ? 0.f : exp2f(-deltaTime / gravityCorrectHalfTime);
					float confidentSmoothCorrect = errorAngle;
					confidentSmoothCorrect *= 1.0f - correctFactor;

					if (TimeCorrecting < gravityCorrectEaseInTime)
					{
						confidentSmoothCorrect *= TimeCorrecting / gravityCorrectEaseInTime;
					}

					Quaternion = AngleAxis(confidentSmoothCorrect * (float)M_PI / 180.0f, flattened.x, flattened.y, flattened.z) * Quaternion;
				}

				Grav = Vec(0.0f, -gravityLength, 0.0f) * Quaternion.Inverse();
				Accel = accel + Grav; // gravity won't be shaky. accel might. so let's keep using the quaternion's calculated gravity vector.
			}
			else
			{
				/*if (TimeCorrecting > 0.f)
				{
					printf("Gravity Shaken\n");
				}/**/

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

	void Motion::SetSettings(GamepadMotionSettings* settings)
	{
		Settings = settings;
	}

	SensorMinMaxWindow::SensorMinMaxWindow()
	{
		Reset(0.f);
	}

	void SensorMinMaxWindow::Reset(float remainder)
	{
		NumSamples = 0;
		TimeSampled = remainder;
	}

	void SensorMinMaxWindow::AddSample(const Vec& inGyro, const Vec& inAccel, float deltaTime)
	{
		if (NumSamples == 0)
		{
			MaxGyro = inGyro;
			MinGyro = inGyro;
			MeanGyro = inGyro;
			MaxAccel = inAccel;
			MinAccel = inAccel;
			MeanAccel = inAccel;
			StartAccel = inAccel;
			NumSamples = 1;
			TimeSampled += deltaTime;
			return;
		}

		MaxGyro = MaxGyro.Max(inGyro);
		MinGyro = MinGyro.Min(inGyro);
		MaxAccel = MaxAccel.Max(inAccel);
		MinAccel = MinAccel.Min(inAccel);

		NumSamples++;
		TimeSampled += deltaTime;

		// https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Welford's_online_algorithm
		Vec delta = inGyro - MeanGyro;
		MeanGyro += delta * (1.f / NumSamples);
		delta = inAccel - MeanAccel;
		MeanAccel += delta * (1.f / NumSamples);
	}

	Vec SensorMinMaxWindow::GetMidGyro()
	{
		return MeanGyro;
	}

	AutoCalibration::AutoCalibration()
	{
		CalibrationData = nullptr;
		MinMaxWindow.TimeSampled = 0.f;
		TimeSteadyStillness = 0.f;
	}

	bool AutoCalibration::AddSampleStillness(const Vec& inGyro, const Vec& inAccel, float deltaTime, bool doSensorFusion)
	{
		if (inGyro.x == 0.f && inGyro.y == 0.f && inGyro.z == 0.f &&
			inAccel.x == 0.f && inAccel.y == 0.f && inAccel.z == 0.f)
		{
			// zeroes are almost certainly not valid inputs
			return false;
		}

		if (!Settings)
		{
			return false;
		}

		// get settings
		const int minStillnessSamples = Settings->MinStillnessSamples;
		const float minStillnessCollectionTime = Settings->MinStillnessCollectionTime;
		const float minStillnessCorrectionTime = Settings->MinStillnessCorrectionTime;
		const float maxStillnessError = Settings->MaxStillnessError;
		const float stillnessSampleDeteriorationRate = Settings->StillnessSampleDeteriorationRate;
		const float stillnessErrorClimbRate = Settings->StillnessErrorClimbRate;
		const float stillnessErrorDropOnRecalibrate = Settings->StillnessErrorDropOnRecalibrate;
		const float stillnessCalibrationEaseInTime = Settings->StillnessCalibrationEaseInTime;
		const float stillnessCalibrationHalfTime = Settings->StillnessCalibrationHalfTime;

		bool calibrated = false;
		const Vec climbThisTick = Vec(stillnessSampleDeteriorationRate * deltaTime);
		MinDeltaGyro += climbThisTick;
		MinDeltaAccel += climbThisTick;

		MinMaxWindow.AddSample(inGyro, inAccel, deltaTime);

		// get deltas
		const Vec gyroDelta = MinMaxWindow.MaxGyro - MinMaxWindow.MinGyro;
		const Vec accelDelta = MinMaxWindow.MaxAccel - MinMaxWindow.MinAccel;

		if (MinMaxWindow.NumSamples >= minStillnessSamples && MinMaxWindow.TimeSampled >= minStillnessCollectionTime)
		{
			MinDeltaGyro = MinDeltaGyro.Min(gyroDelta);
			MinDeltaAccel = MinDeltaAccel.Min(accelDelta);
		}
		else
		{
			RecalibrateThreshold = min(RecalibrateThreshold + stillnessErrorClimbRate * deltaTime, maxStillnessError);
			return false;
		}

		// check that all inputs are below appropriate thresholds to be considered "still"
		if (gyroDelta.x <= MinDeltaGyro.x * RecalibrateThreshold &&
			gyroDelta.y <= MinDeltaGyro.y * RecalibrateThreshold &&
			gyroDelta.z <= MinDeltaGyro.z * RecalibrateThreshold &&
			accelDelta.x <= MinDeltaAccel.x * RecalibrateThreshold &&
			accelDelta.y <= MinDeltaAccel.y * RecalibrateThreshold &&
			accelDelta.z <= MinDeltaAccel.z * RecalibrateThreshold)
		{
			if (CalibrationData != nullptr && MinMaxWindow.NumSamples >= minStillnessSamples && MinMaxWindow.TimeSampled >= minStillnessCorrectionTime)
			{
				/*if (TimeSteadyStillness == 0.f)
				{
					printf("Still!\n");
				}/**/

				TimeSteadyStillness = min(TimeSteadyStillness + deltaTime, stillnessCalibrationEaseInTime);
				const float calibrationEaseIn = stillnessCalibrationEaseInTime <= 0.f ? 1.f : TimeSteadyStillness / stillnessCalibrationEaseInTime;

				const Vec calibratedGyro = MinMaxWindow.GetMidGyro();

				const Vec oldGyroBias = Vec(CalibrationData->X, CalibrationData->Y, CalibrationData->Z) / max((float)CalibrationData->NumSamples, 1.f);
				const float stillnessLerpFactor = stillnessCalibrationHalfTime <= 0.f ? 0.f : exp2f(-calibrationEaseIn * deltaTime / stillnessCalibrationHalfTime);
				Vec newGyroBias = calibratedGyro.Lerp(oldGyroBias, stillnessLerpFactor);

				if (doSensorFusion)
				{
					const Vec previousNormal = MinMaxWindow.StartAccel.Normalized();
					const Vec thisNormal = inAccel.Normalized();
					Vec angularVelocity = thisNormal.Cross(previousNormal);
					const float crossLength = angularVelocity.Length();
					if (crossLength > 0.f)
					{
						const float thisDotPrev = min(max(-1.f, thisNormal.Dot(previousNormal)), 1.f);
						const float angleChange = acosf(thisDotPrev) * 180.0f / (float)M_PI;
						const float anglePerSecond = angleChange / MinMaxWindow.TimeSampled;
						angularVelocity *= anglePerSecond / crossLength;
					}

					Vec axisCalibrationStrength = thisNormal.Abs();
					Vec sensorFusionBias = (calibratedGyro - angularVelocity).Lerp(oldGyroBias, stillnessLerpFactor);
					if (axisCalibrationStrength.x <= 0.7f)
					{
						newGyroBias.x = sensorFusionBias.x;
					}
					if (axisCalibrationStrength.y <= 0.7f)
					{
						newGyroBias.y = sensorFusionBias.y;
					}
					if (axisCalibrationStrength.z <= 0.7f)
					{
						newGyroBias.z = sensorFusionBias.z;
					}
				}

				CalibrationData->X = newGyroBias.x;
				CalibrationData->Y = newGyroBias.y;
				CalibrationData->Z = newGyroBias.z;

				CalibrationData->AccelMagnitude = MinMaxWindow.MeanAccel.Length();
				CalibrationData->NumSamples = 1;

				calibrated = true;
			}
			else
			{
				RecalibrateThreshold = min(RecalibrateThreshold + stillnessErrorClimbRate * deltaTime, maxStillnessError);
			}
		}
		else if (TimeSteadyStillness > 0.f)
		{
			//printf("Moved!\n");
			RecalibrateThreshold -= stillnessErrorDropOnRecalibrate;
			if (RecalibrateThreshold < 1.f) RecalibrateThreshold = 1.f;

			TimeSteadyStillness = 0.f;
			MinMaxWindow.Reset(0.f);
		}
		else
		{
			RecalibrateThreshold = min(RecalibrateThreshold + stillnessErrorClimbRate * deltaTime, maxStillnessError);
			MinMaxWindow.Reset(0.f);
		}

		return calibrated;
	}

	void AutoCalibration::NoSampleStillness()
	{
		MinMaxWindow.Reset(0.f);
	}

	bool AutoCalibration::AddSampleSensorFusion(const Vec& inGyro, const Vec& inAccel, float deltaTime)
	{
		if (deltaTime <= 0.f)
		{
			return false;
		}

		if (inGyro.x == 0.f && inGyro.y == 0.f && inGyro.z == 0.f &&
			inAccel.x == 0.f && inAccel.y == 0.f && inAccel.z == 0.f)
		{
			// all zeroes are almost certainly not valid inputs
			TimeSteadySensorFusion = 0.f;
			SensorFusionSkippedTime = 0.f;
			PreviousAccel = inAccel;
			SmoothedPreviousAccel = inAccel;
			SmoothedAngularVelocityGyro = GamepadMotionHelpers::Vec();
			SmoothedAngularVelocityAccel = GamepadMotionHelpers::Vec();
			return false;
		}

		if (PreviousAccel.x == 0.f && PreviousAccel.y == 0.f && PreviousAccel.z == 0.f)
		{
			TimeSteadySensorFusion = 0.f;
			SensorFusionSkippedTime = 0.f;
			PreviousAccel = inAccel;
			SmoothedPreviousAccel = inAccel;
			SmoothedAngularVelocityGyro = GamepadMotionHelpers::Vec();
			SmoothedAngularVelocityAccel = GamepadMotionHelpers::Vec();
			return false;
		}

		// in case the controller state hasn't updated between samples
		if (inAccel.x == PreviousAccel.x && inAccel.y == PreviousAccel.y && inAccel.z == PreviousAccel.z)
		{
			SensorFusionSkippedTime += deltaTime;
			return false;
		}

		if (!Settings)
		{
			return false;
		}

		// get settings
		const float sensorFusionCalibrationSmoothingStrength = Settings->SensorFusionCalibrationSmoothingStrength;
		const float sensorFusionAngularAccelerationThreshold = Settings->SensorFusionAngularAccelerationThreshold;
		const float sensorFusionCalibrationEaseInTime = Settings->SensorFusionCalibrationEaseInTime;
		const float sensorFusionCalibrationHalfTime = Settings->SensorFusionCalibrationHalfTime;

		deltaTime += SensorFusionSkippedTime;
		SensorFusionSkippedTime = 0.f;
		bool calibrated = false;
		
		// framerate independent lerp smoothing: https://www.gamasutra.com/blogs/ScottLembcke/20180404/316046/Improved_Lerp_Smoothing.php
		const float smoothingLerpFactor = exp2f(-sensorFusionCalibrationSmoothingStrength * deltaTime);
		// velocity from smoothed accel matches better if we also smooth gyro
		const Vec previousGyro = SmoothedAngularVelocityGyro;
		SmoothedAngularVelocityGyro = inGyro.Lerp(SmoothedAngularVelocityGyro, smoothingLerpFactor); // smooth what remains
		const float gyroAccelerationMag = (SmoothedAngularVelocityGyro - previousGyro).Length() / deltaTime;
		// get angle between old and new accel
		const Vec previousNormal = SmoothedPreviousAccel.Normalized();
		const Vec thisAccel = inAccel.Lerp(SmoothedPreviousAccel, smoothingLerpFactor);
		const Vec thisNormal = thisAccel.Normalized();
		Vec angularVelocity = thisNormal.Cross(previousNormal);
		const float crossLength = angularVelocity.Length();
		if (crossLength > 0.f)
		{
			const float thisDotPrev = min(max(-1.f, thisNormal.Dot(previousNormal)), 1.f);
			const float angleChange = acosf(thisDotPrev) * 180.0f / (float)M_PI;
			const float anglePerSecond = angleChange / deltaTime;
			angularVelocity *= anglePerSecond / crossLength;
		}
		SmoothedAngularVelocityAccel = angularVelocity;

		// apply corrections
		if (gyroAccelerationMag > sensorFusionAngularAccelerationThreshold || CalibrationData == nullptr)
		{
			/*if (TimeSteadySensorFusion > 0.f)
			{
				printf("Shaken!\n");
			}/**/
			TimeSteadySensorFusion = 0.f;
			//printf("No calibration due to acceleration of %.4f\n", gyroAccelerationMag);
		}
		else
		{
			/*if (TimeSteadySensorFusion == 0.f)
			{
				printf("Steady!\n");
			}/**/

			TimeSteadySensorFusion = min(TimeSteadySensorFusion + deltaTime, sensorFusionCalibrationEaseInTime);
			const float calibrationEaseIn = sensorFusionCalibrationEaseInTime <= 0.f ? 1.f : TimeSteadySensorFusion / sensorFusionCalibrationEaseInTime;
			const Vec oldGyroBias = Vec(CalibrationData->X, CalibrationData->Y, CalibrationData->Z) / max((float)CalibrationData->NumSamples, 1.f);
			// recalibrate over time proportional to the difference between the calculated bias and the current assumed bias
			const float sensorFusionLerpFactor = sensorFusionCalibrationHalfTime <= 0.f ? 0.f : exp2f(-calibrationEaseIn * deltaTime / sensorFusionCalibrationHalfTime);
			Vec newGyroBias = (SmoothedAngularVelocityGyro - SmoothedAngularVelocityAccel).Lerp(oldGyroBias, sensorFusionLerpFactor);
			// don't change bias in axes that can't be affected by the gravity direction
			Vec axisCalibrationStrength = thisNormal.Abs();
			if (axisCalibrationStrength.x > 0.7f)
			{
				axisCalibrationStrength.x = 1.f;
			}
			if (axisCalibrationStrength.y > 0.7f)
			{
				axisCalibrationStrength.y = 1.f;
			}
			if (axisCalibrationStrength.z > 0.7f)
			{
				axisCalibrationStrength.z = 1.f;
			}
			newGyroBias = newGyroBias.Lerp(oldGyroBias, axisCalibrationStrength.Min(Vec(1.f)));

			CalibrationData->X = newGyroBias.x;
			CalibrationData->Y = newGyroBias.y;
			CalibrationData->Z = newGyroBias.z;

			CalibrationData->AccelMagnitude = thisAccel.Length();

			CalibrationData->NumSamples = 1;

			calibrated = true;

			//printf("Recalibrating at a strength of %.4f\n", calibrationEaseIn);
		}

		SmoothedPreviousAccel = thisAccel;
		PreviousAccel = inAccel;

		//printf("Gyro: %.4f, %.4f, %.4f | Accel: %.4f, %.4f, %.4f\n",
		//	SmoothedAngularVelocityGyro.x, SmoothedAngularVelocityGyro.y, SmoothedAngularVelocityGyro.z,
		//	SmoothedAngularVelocityAccel.x, SmoothedAngularVelocityAccel.y, SmoothedAngularVelocityAccel.z);

		return calibrated;
	}

	void AutoCalibration::NoSampleSensorFusion()
	{
		TimeSteadySensorFusion = 0.f;
		SensorFusionSkippedTime = 0.f;
		PreviousAccel = GamepadMotionHelpers::Vec();
		SmoothedPreviousAccel = GamepadMotionHelpers::Vec();
		SmoothedAngularVelocityGyro = GamepadMotionHelpers::Vec();
		SmoothedAngularVelocityAccel = GamepadMotionHelpers::Vec();
	}

	void AutoCalibration::SetCalibrationData(GyroCalibration* calibrationData)
	{
		CalibrationData = calibrationData;
	}

	void AutoCalibration::SetSettings(GamepadMotionSettings* settings)
	{
		Settings = settings;
	}

} // namespace GamepadMotionHelpers

GamepadMotion::GamepadMotion()
{
	IsCalibrating = false;
	CurrentCalibrationMode = GamepadMotionHelpers::CalibrationMode::Manual;
	Reset();
	AutoCalibration.SetCalibrationData(&GyroCalibration);
	AutoCalibration.SetSettings(&Settings);
	Motion.SetSettings(&Settings);
}

void GamepadMotion::Reset()
{
	GyroCalibration = {};
	Gyro = {};
	RawAccel = {};
	Settings = GamepadMotionSettings();
	Motion.Reset();
}

void GamepadMotion::ProcessMotion(float gyroX, float gyroY, float gyroZ,
	float accelX, float accelY, float accelZ, float deltaTime)
{
	float accelMagnitude = sqrtf(accelX * accelX + accelY * accelY + accelZ * accelZ);

	if (IsCalibrating)
	{
		// manual calibration
		PushSensorSamples(gyroX, gyroY, gyroZ, accelMagnitude);
		AutoCalibration.NoSampleSensorFusion();
		AutoCalibration.NoSampleStillness();
	}
	else if (CurrentCalibrationMode & GamepadMotionHelpers::CalibrationMode::Stillness)
	{
		AutoCalibration.AddSampleStillness(GamepadMotionHelpers::Vec(gyroX, gyroY, gyroZ), GamepadMotionHelpers::Vec(accelX, accelY, accelZ), deltaTime, CurrentCalibrationMode & GamepadMotionHelpers::CalibrationMode::SensorFusion);
		AutoCalibration.NoSampleSensorFusion();
	}
	else
	{
		AutoCalibration.NoSampleStillness();
		if (CurrentCalibrationMode & GamepadMotionHelpers::CalibrationMode::SensorFusion)
		{
			AutoCalibration.AddSampleSensorFusion(GamepadMotionHelpers::Vec(gyroX, gyroY, gyroZ), GamepadMotionHelpers::Vec(accelX, accelY, accelZ), deltaTime);
		}
		else
		{
			AutoCalibration.NoSampleSensorFusion();
		}
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
		GyroCalibration.AccelMagnitude = (float)weight;
	}

	GyroCalibration.NumSamples = weight;
	GyroCalibration.X = xOffset * weight;
	GyroCalibration.Y = yOffset * weight;
	GyroCalibration.Z = zOffset * weight;
}

GamepadMotionHelpers::CalibrationMode GamepadMotion::GetCalibrationMode()
{
	return CurrentCalibrationMode;
}

void GamepadMotion::SetCalibrationMode(GamepadMotionHelpers::CalibrationMode calibrationMode)
{
	CurrentCalibrationMode = calibrationMode;
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

	const float inverseSamples = 1.f / GyroCalibration.NumSamples;
	gyroOffsetX = GyroCalibration.X * inverseSamples;
	gyroOffsetY = GyroCalibration.Y * inverseSamples;
	gyroOffsetZ = GyroCalibration.Z * inverseSamples;
	accelMagnitude = GyroCalibration.AccelMagnitude * inverseSamples;
}
