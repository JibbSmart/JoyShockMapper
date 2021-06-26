#include "MotionIf.h"
#include "GamepadMotion.hpp"

class MotionImpl : public MotionIf
{
	GamepadMotion gamepadMotion;
public:
	MotionImpl()
	  : gamepadMotion()
	{
	}
	
	virtual ~MotionImpl()
	{

	}
	
	virtual void Reset() override 
	{
		gamepadMotion.Reset();
	}

	virtual void ProcessMotion(float gyroX, float gyroY, float gyroZ,
	  float accelX, float accelY, float accelZ, float deltaTime) override 
	{
		gamepadMotion.ProcessMotion(gyroX, gyroY, gyroZ, accelX, accelY, accelZ, deltaTime);
	}

	// reading the current state
	virtual void GetCalibratedGyro(float& x, float& y, float& z) override 
	{
		gamepadMotion.GetCalibratedGyro(x, y, z);
	}

	virtual void GetGravity(float& x, float& y, float& z) override 
	{
		gamepadMotion.GetGravity(x, y, z);
	}

	virtual void GetProcessedAcceleration(float& x, float& y, float& z) override 
	{
		gamepadMotion.GetProcessedAcceleration(x, y, z);
	}

	virtual void GetOrientation(float& w, float& x, float& y, float& z) override 
	{
		gamepadMotion.GetOrientation(w, x, y, z);
	}

	// gyro calibration functions
	virtual void StartContinuousCalibration() override 
	{
		gamepadMotion.StartContinuousCalibration();
	}

	virtual void PauseContinuousCalibration() override 
	{
		gamepadMotion.PauseContinuousCalibration();
	}

	virtual void ResetContinuousCalibration() override 
	{
		gamepadMotion.ResetContinuousCalibration();
	}

	virtual void GetCalibrationOffset(float& xOffset, float& yOffset, float& zOffset) override 
	{
		gamepadMotion.GetCalibrationOffset(xOffset, yOffset, zOffset);
	}

	virtual void SetCalibrationOffset(float xOffset, float yOffset, float zOffset, int weight) override 
	{
		gamepadMotion.SetCalibrationOffset(xOffset, yOffset, zOffset, weight);
	}

	virtual void SetAutoCalibration(bool enabled, float gyroThreshold, float accelThreshold) override
	{
		if (enabled)
		{
			gamepadMotion.SetCalibrationMode(GamepadMotionHelpers::CalibrationMode::Stillness | GamepadMotionHelpers::CalibrationMode::SensorFusion);
			gamepadMotion.Settings.StillnessGyroDelta = gyroThreshold;
			gamepadMotion.Settings.StillnessAccelDelta = accelThreshold;
		}
		else
		{
			gamepadMotion.SetCalibrationMode(GamepadMotionHelpers::CalibrationMode::Manual);
		}
	}

	void virtual ResetMotion() override 
	{
		gamepadMotion.ResetMotion();
	}
};

MotionIf* MotionIf::getNew()
{
	return new MotionImpl();
}