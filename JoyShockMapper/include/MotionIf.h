#pragma once

class MotionIf
{
protected:
	MotionIf(){};
public:
	static MotionIf* getNew();
	virtual ~MotionIf() {};
	

	virtual void Reset() = 0;

	virtual void ProcessMotion(float gyroX, float gyroY, float gyroZ,
	  float accelX, float accelY, float accelZ, float deltaTime) = 0;

	// reading the current state
	virtual void GetCalibratedGyro(float& x, float& y, float& z) = 0;
	virtual void GetGravity(float& x, float& y, float& z) = 0;
	virtual void GetProcessedAcceleration(float& x, float& y, float& z) = 0;
	virtual void GetOrientation(float& w, float& x, float& y, float& z) = 0;

	// gyro calibration functions
	virtual void StartContinuousCalibration() = 0;
	virtual void PauseContinuousCalibration() = 0;
	virtual void ResetContinuousCalibration() = 0;
	virtual void GetCalibrationOffset(float& xOffset, float& yOffset, float& zOffset) = 0;
	virtual void SetCalibrationOffset(float xOffset, float yOffset, float zOffset, int weight) = 0;

	void virtual ResetMotion() = 0;
};
