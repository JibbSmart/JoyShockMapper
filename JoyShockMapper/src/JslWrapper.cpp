#define JSL_WRAPPER_SOURCE
#include "JslWrapper.h"

class JSlWrapperImpl : public JslWrapper
{
public:
	int ConnectDevices() override
	{
		return JslConnectDevices();
	}

	int GetConnectedDeviceHandles(int* deviceHandleArray, int size) override
	{
		return JslGetConnectedDeviceHandles(deviceHandleArray, size);
	}

	void DisconnectAndDisposeAll() override
	{
		return JslDisconnectAndDisposeAll();
	}

	JOY_SHOCK_STATE GetSimpleState(int deviceId) override
	{
		return JslGetSimpleState(deviceId);
	}

	IMU_STATE GetIMUState(int deviceId) override
	{
		return JslGetIMUState(deviceId);
	}

	MOTION_STATE GetMotionState(int deviceId) override
	{
		return JslGetMotionState(deviceId);
	}

	TOUCH_STATE GetTouchState(int deviceId, bool previous = false) override
	{
		return JslGetTouchState(deviceId, previous);
	}

	bool GetTouchpadDimension(int deviceId, int& sizeX, int& sizeY) override
	{
		return JslGetTouchpadDimension(deviceId, sizeX, sizeY);
	}

	int GetButtons(int deviceId) override
	{
		return JslGetButtons(deviceId);
	}

	float GetLeftX(int deviceId) override
	{
		return JslGetLeftX(deviceId);
	}

	float GetLeftY(int deviceId) override
	{
		return JslGetLeftY(deviceId);
	}

	float GetRightX(int deviceId) override
	{
		return JslGetRightX(deviceId);
	}

	float GetRightY(int deviceId) override
	{
		return JslGetLeftX(deviceId);
	}

	float GetLeftTrigger(int deviceId) override
	{
		return JslGetLeftTrigger(deviceId);
	}

	float GetRightTrigger(int deviceId) override
	{
		return JslGetRightTrigger(deviceId);
	}

	float GetGyroX(int deviceId) override
	{
		return JslGetGyroX(deviceId);
	}

	float GetGyroY(int deviceId) override
	{
		return JslGetGyroY(deviceId);
	}

	float GetGyroZ(int deviceId) override
	{
		return JslGetGyroZ(deviceId);
	}

	float GetAccelX(int deviceId) override
	{
		return JslGetAccelX(deviceId);
	}

	float GetAccelY(int deviceId) override
	{
		return JslGetAccelY(deviceId);
	}

	float GetAccelZ(int deviceId) override
	{
		return JslGetAccelZ(deviceId);
	}

	int GetTouchId(int deviceId, bool secondTouch = false) override
	{
		return JslGetTouchId(deviceId, secondTouch);
	}

	bool GetTouchDown(int deviceId, bool secondTouch = false) override
	{
		return JslGetTouchDown(deviceId, secondTouch);
	}

	float GetTouchX(int deviceId, bool secondTouch = false) override
	{
		return JslGetTouchX(deviceId, secondTouch);
	}

	float GetTouchY(int deviceId, bool secondTouch = false) override
	{
		return JslGetTouchY(deviceId, secondTouch);
	}

	float GetStickStep(int deviceId) override
	{
		return JslGetStickStep(deviceId);
	}

	float GetTriggerStep(int deviceId) override
	{
		return JslGetTriggerStep(deviceId);
	}

	float GetPollRate(int deviceId) override
	{
		return JslGetPollRate(deviceId);
	}

	void ResetContinuousCalibration(int deviceId) override
	{
		JslResetContinuousCalibration(deviceId);
	}

	void StartContinuousCalibration(int deviceId) override
	{
		JslStartContinuousCalibration(deviceId);
	}

	void PauseContinuousCalibration(int deviceId) override
	{
		return JslPauseContinuousCalibration(deviceId);
	}

	void GetCalibrationOffset(int deviceId, float& xOffset, float& yOffset, float& zOffset) override
	{
		JslGetCalibrationOffset(deviceId, xOffset, yOffset, zOffset);
	}

	void SetCalibrationOffset(int deviceId, float xOffset, float yOffset, float zOffset) override
	{
		JslSetCalibrationOffset(deviceId, xOffset, yOffset, zOffset);
	}

	void SetCallback(void (*callback)(int, JOY_SHOCK_STATE, JOY_SHOCK_STATE, IMU_STATE, IMU_STATE, float)) override
	{
		JslSetCallback(callback);
	}

	void SetTouchCallback(void (*callback)(int, TOUCH_STATE, TOUCH_STATE, float)) override
	{
		JslSetTouchCallback(callback);
	}

	int GetControllerType(int deviceId) override
	{
		return JslGetControllerType(deviceId);
	}

	int GetControllerSplitType(int deviceId) override
	{
		return JslGetControllerType(deviceId);
	}

	int GetControllerColour(int deviceId) override
	{
		return JslGetControllerColour(deviceId);
	}

	void SetLightColour(int deviceId, int colour) override
	{
		JslSetLightColour(deviceId, colour);
	}

	void SetRumble(int deviceId, int smallRumble, int bigRumble) override
	{
		JslSetRumble(deviceId, smallRumble, bigRumble);
	}

	void SetPlayerNumber(int deviceId, int number) override
	{
		JslSetPlayerNumber(deviceId, number);
	}

	void SetLeftTriggerEffect(int deviceId, int triggerEffect)
	{
		// Unsupported ATM
	}

	void SetRightTriggerEffect(int deviceId, int triggerEffect)
	{
		// Unsupported ATM
	}
};

JslWrapper* JslWrapper::getNew()
{
	return new JSlWrapperImpl();
}
