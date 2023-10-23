module;

#include <cstdint>

export module TriggerEffectGenerator;

/*
 * MIT License
 *
 * Copyright (c) 2021 John "Nielk1" Klein
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * This file is the CPP-fication of Nielk1's ExtendInput.DataTools.DualSense.TriggerEffectGenerator.cs
 * found at https://gist.github.com/Nielk1/6d54cc2c00d2201ccb8c2720ad7538db under
 * the same MIT license.
 */

enum class TriggerEffectType : uint8_t
{
	SimpleResistance = 0x01,       // 00 00 0 001 // Suggest don't use
	SimpleSemiAutomaticGun = 0x02, // 00 00 0 100 // Suggest don't use
	reset = 0x05,                  // 00 00 0 101 // Safe to Use, part of libpad
	SimpleAutomaticGun = 0x06,     // 00 00 0 110 // Suggest don't use

	LimitedResistance = 0x11,       // 00 01 0 001 // Suggest don't use
	LimitedSemiAutomaticGun = 0x12, // 00 01 0 010 // Suggest don't use

	Resistance = 0x21,       // 00 10 0 001 // Safe to Use, part of libpad
	Bow = 0x22,              // 00 10 0 010 // Suspect, part of safe block
	Galloping = 0x23,        // 00 10 0 011 // Suspect, part of safe block
	SemiAutomaticGun = 0x25, // 00 10 0 101 // Safe to Use, part of libpad
	AutomaticGun = 0x26,     // 00 10 0 110 // Safe to Use, part of libpad
	Machine = 0x27,          // 00 10 0 111 // Suspect, part of safe block

	DebugFC = 0xFC, // 11 11 1 100
	DebugFD = 0xFD, // 11 11 1 101
	DebugFE = 0xFE, // 11 11 1 110
};

export namespace ExtendInput::DataTools::DualSense
{

/// DualSense controller trigger effect generators.
/// Revision: 1
class TriggerEffectGenerator
{
public:
	/// Simplistic resistance effect data generator.
	/// This is not used by libpad and may be removed in a future DualSense firmware.
	/// @note Use <see cref="Resistance(uint8_t[], int, uint8_t, uint8_t)"/> instead.
	/// @param destinationArray The uint8_t[] that receives the data.
	/// @param destinationIndex A 32-bit integer that represents the index in the destinationArray at which storing begins.
	/// @param start The starting zone of the trigger effect.
	/// @param force The force of the resistance.
	/// @returns The success of the effect write.
	static bool SimpleResistance(uint8_t *destinationArray, int destinationIndex, uint8_t start, uint8_t force)
	{
		destinationArray[destinationIndex + 0] = (uint8_t)TriggerEffectType::SimpleResistance;
		destinationArray[destinationIndex + 1] = start;
		destinationArray[destinationIndex + 2] = force;
		destinationArray[destinationIndex + 3] = 0x00;
		destinationArray[destinationIndex + 4] = 0x00;
		destinationArray[destinationIndex + 5] = 0x00;
		destinationArray[destinationIndex + 6] = 0x00;
		destinationArray[destinationIndex + 7] = 0x00;
		destinationArray[destinationIndex + 8] = 0x00;
		destinationArray[destinationIndex + 9] = 0x00;
		destinationArray[destinationIndex + 10] = 0x00;
		return true;
	}

	/// Simplistic semi-automatic gun effect data generator.
	/// This is not used by libpad and may be removed in a future DualSense firmware.
	/// @note Use <see cref="SemiAutomaticGun(uint8_t[], int, uint8_t, uint8_t, uint8_t)"/> instead.
	/// @param destinationArray The uint8_t[] that receives the data.
	/// @param destinationIndex A 32-bit integer that represents the index in the destinationArray at which storing begins.
	/// @param start The starting zone of the trigger effect.
	/// @param end The ending zone of the trigger effect.
	/// @param force The force of the resistance.
	/// @returns The success of the effect write.
	static bool SimpleSemiAutomaticGun(uint8_t *destinationArray, int destinationIndex, uint8_t start, uint8_t end, uint8_t force)
	{
		destinationArray[destinationIndex + 0] = (uint8_t)TriggerEffectType::SimpleSemiAutomaticGun;
		destinationArray[destinationIndex + 1] = start;
		destinationArray[destinationIndex + 2] = end;
		destinationArray[destinationIndex + 3] = force;
		destinationArray[destinationIndex + 4] = 0x00;
		destinationArray[destinationIndex + 5] = 0x00;
		destinationArray[destinationIndex + 6] = 0x00;
		destinationArray[destinationIndex + 7] = 0x00;
		destinationArray[destinationIndex + 8] = 0x00;
		destinationArray[destinationIndex + 9] = 0x00;
		destinationArray[destinationIndex + 10] = 0x00;
		return true;
	}

	/// reset effect data generator.
	/// This is used by libpad and is expected to be present in future DualSense firmware.
	/// @param destinationArray The uint8_t[] that receives the data.
	/// @param destinationIndex A 32-bit integer that represents the index in the destinationArray at which storing begins.
	/// @returns The success of the effect write.
	static bool reset(uint8_t *destinationArray, int destinationIndex)
	{
		destinationArray[destinationIndex + 0] = (uint8_t)TriggerEffectType::reset;
		destinationArray[destinationIndex + 1] = 0x00;
		destinationArray[destinationIndex + 2] = 0x00;
		destinationArray[destinationIndex + 3] = 0x00;
		destinationArray[destinationIndex + 4] = 0x00;
		destinationArray[destinationIndex + 5] = 0x00;
		destinationArray[destinationIndex + 6] = 0x00;
		destinationArray[destinationIndex + 7] = 0x00;
		destinationArray[destinationIndex + 8] = 0x00;
		destinationArray[destinationIndex + 9] = 0x00;
		destinationArray[destinationIndex + 10] = 0x00;
		return true;
	}

	/// Simplistic automatic gun effect data generator.
	/// This is not used by libpad and may be removed in a future DualSense firmware.
	/// @note Use <see cref="AutomaticGun(uint8_t[], int, uint8_t, uint8_t, uint8_t)"/> instead.
	/// @param destinationArray The uint8_t[] that receives the data.
	/// @param destinationIndex A 32-bit integer that represents the index in the destinationArray at which storing begins.
	/// @param start The starting zone of the trigger effect.
	/// @param force Strength of the automatic cycling action.
	/// @param frequency Frequency of the automatic cycling action in hertz.
	/// @returns The success of the effect write.
	static bool SimpleAutomaticGun(uint8_t *destinationArray, int destinationIndex, uint8_t start, uint8_t strength, uint8_t frequency)
	{
		if (frequency > 0 && strength > 0)
		{
			destinationArray[destinationIndex + 0] = (uint8_t)TriggerEffectType::SimpleAutomaticGun;
			destinationArray[destinationIndex + 1] = frequency;
			destinationArray[destinationIndex + 2] = strength;
			destinationArray[destinationIndex + 3] = start;
			destinationArray[destinationIndex + 4] = 0x00;
			destinationArray[destinationIndex + 5] = 0x00;
			destinationArray[destinationIndex + 6] = 0x00;
			destinationArray[destinationIndex + 7] = 0x00;
			destinationArray[destinationIndex + 8] = 0x00;
			destinationArray[destinationIndex + 9] = 0x00;
			destinationArray[destinationIndex + 10] = 0x00;
			return true;
		}
		return reset(destinationArray, destinationIndex);
	}

	/// Simplistic resistance effect data generator with stricter paramater limits.
	/// This is not used by libpad and may be removed in a future DualSense firmware.
	/// @note <see cref="Resistance(uint8_t[], int, uint8_t, uint8_t)"/> instead.
	/// @param destinationArray The uint8_t[] that receives the data.
	/// @param destinationIndex A 32-bit integer that represents the index in the destinationArray at which storing begins.
	/// @param start The starting zone of the trigger effect.
	/// @param force The force of the resistance. Must be between 0 and 10 inclusive.
	/// @returns The success of the effect write.
	static bool LimitedResistance(uint8_t *destinationArray, int destinationIndex, uint8_t start, uint8_t force)
	{
		if (force > 10)
			return false;
		if (force > 0)
		{
			destinationArray[destinationIndex + 0] = (uint8_t)TriggerEffectType::LimitedResistance;
			destinationArray[destinationIndex + 1] = start;
			destinationArray[destinationIndex + 2] = force;
			destinationArray[destinationIndex + 3] = 0x00;
			destinationArray[destinationIndex + 4] = 0x00;
			destinationArray[destinationIndex + 5] = 0x00;
			destinationArray[destinationIndex + 6] = 0x00;
			destinationArray[destinationIndex + 7] = 0x00;
			destinationArray[destinationIndex + 8] = 0x00;
			destinationArray[destinationIndex + 9] = 0x00;
			destinationArray[destinationIndex + 10] = 0x00;
			return true;
		}
		return reset(destinationArray, destinationIndex);
	}

	/// Simplistic semi-automatic gun effect data generator with stricter paramater limits.
	/// This is not used by libpad and may be removed in a future DualSense firmware.
	/// @note <see cref="SemiAutomaticGun(uint8_t[], int, uint8_t, uint8_t, uint8_t)"/> instead.
	/// @param destinationArray The uint8_t[] that receives the data.
	/// @param destinationIndex A 32-bit integer that represents the index in the destinationArray at which storing begins.
	/// @param start The starting zone of the trigger effect. Must be 16 or higher.
	/// @param end The ending zone of the trigger effect. Must be between <paramref name="start"/> and <paramref name="start"/>+100 inclusive.
	/// @param force The force of the resistance. Must be between 0 and 10 inclusive.
	/// @returns The success of the effect write.
	static bool LimitedSemiAutomaticGun(uint8_t *destinationArray, int destinationIndex, uint8_t start, uint8_t end, uint8_t force)
	{
		if (start < 0x10)
			return false;
		if (end < start || (start + 100) < end)
			return false;
		if (force > 10)
			return false;
		if (force > 0)
		{
			destinationArray[destinationIndex + 0] = (uint8_t)TriggerEffectType::LimitedSemiAutomaticGun;
			destinationArray[destinationIndex + 1] = start;
			destinationArray[destinationIndex + 2] = end;
			destinationArray[destinationIndex + 3] = force;
			destinationArray[destinationIndex + 4] = 0x00;
			destinationArray[destinationIndex + 5] = 0x00;
			destinationArray[destinationIndex + 6] = 0x00;
			destinationArray[destinationIndex + 7] = 0x00;
			destinationArray[destinationIndex + 8] = 0x00;
			destinationArray[destinationIndex + 9] = 0x00;
			destinationArray[destinationIndex + 10] = 0x00;
			return true;
		}
		return reset(destinationArray, destinationIndex);
	}

	/// Resistance effect data generator.
	/// This is used by libpad and is expected to be present in future DualSense firmware.
	/// @param destinationArray The uint8_t[] that receives the data.
	/// @param destinationIndex A 32-bit integer that represents the index in the destinationArray at which storing begins.
	/// @param start The starting zone of the trigger effect. Must be between 0 and 9 inclusive.
	/// @param force The force of the resistance. Must be between 0 and 8 inclusive.
	/// @returns The success of the effect write.
	static bool Resistance(uint8_t *destinationArray, int destinationIndex, uint8_t start, uint8_t force)
	{
		if (start > 9)
			return false;
		if (force > 8)
			return false;
		if (force > 0)
		{
			uint8_t forceValue = (uint8_t)((force - 1) & 0x07);
			uint32_t forceZones = 0;
			uint16_t activeZones = 0;
			for (int i = start; i < 10; i++)
			{
				forceZones |= (uint32_t)(forceValue << (3 * i));
				activeZones |= (uint16_t)(1 << i);
			}

			destinationArray[destinationIndex + 0] = (uint8_t)TriggerEffectType::Resistance;
			destinationArray[destinationIndex + 1] = (uint8_t)((activeZones >> 0) & 0xff);
			destinationArray[destinationIndex + 2] = (uint8_t)((activeZones >> 8) & 0xff);
			destinationArray[destinationIndex + 3] = (uint8_t)((forceZones >> 0) & 0xff);
			destinationArray[destinationIndex + 4] = (uint8_t)((forceZones >> 8) & 0xff);
			destinationArray[destinationIndex + 5] = (uint8_t)((forceZones >> 16) & 0xff);
			destinationArray[destinationIndex + 6] = (uint8_t)((forceZones >> 24) & 0xff);
			destinationArray[destinationIndex + 7] = 0x00; // (uint8_t)((forceZones >> 32) & 0xff); // need 64bit for this, but we already have enough space
			destinationArray[destinationIndex + 8] = 0x00; // (uint8_t)((forceZones >> 40) & 0xff); // need 64bit for this, but we already have enough space
			destinationArray[destinationIndex + 9] = 0x00;
			destinationArray[destinationIndex + 10] = 0x00;
			return true;
		}
		return reset(destinationArray, destinationIndex);
	}

	/// Bow effect data generator.
	/// This is not used by libpad but is in the used effect block, it may be removed in a future DualSense firmware.
	/// @param destinationArray The uint8_t[] that receives the data.
	/// @param destinationIndex A 32-bit integer that represents the index in the destinationArray at which storing begins.
	/// @param start The starting zone of the trigger effect. Must be between 0 and 8 inclusive.
	/// @param end The ending zone of the trigger effect. Must be between <paramref name="start"/> + 1 and 8 inclusive.
	/// @param force The force of the resistance. Must be between 0 and 8 inclusive.
	/// @param snapForce The force of the snap-back. Must be between 0 and 8 inclusive.
	/// @returns The success of the effect write.
	static bool Bow(uint8_t *destinationArray, int destinationIndex, uint8_t start, uint8_t end, uint8_t force, uint8_t snapForce)
	{
		if (start > 8)
			return false;
		if (end > 8)
			return false;
		if (start >= end)
			return false;
		if (force > 8)
			return false;
		if (snapForce > 8)
			return false;
		if (end > 0 && force > 0 && snapForce > 0)
		{
			uint16_t startAndStopZones = (uint16_t)((1 << start) | (1 << end));
			uint32_t forcePair = (uint32_t)((((force - 1) & 0x07) << (3 * 0)) | (((snapForce - 1) & 0x07) << (3 * 1)));

			destinationArray[destinationIndex + 0] = (uint8_t)TriggerEffectType::Bow;
			destinationArray[destinationIndex + 1] = (uint8_t)((startAndStopZones >> 0) & 0xff);
			destinationArray[destinationIndex + 2] = (uint8_t)((startAndStopZones >> 8) & 0xff);
			destinationArray[destinationIndex + 3] = (uint8_t)((forcePair >> 0) & 0xff);
			destinationArray[destinationIndex + 4] = (uint8_t)((forcePair >> 8) & 0xff);
			destinationArray[destinationIndex + 5] = 0x00;
			destinationArray[destinationIndex + 6] = 0x00;
			destinationArray[destinationIndex + 7] = 0x00;
			destinationArray[destinationIndex + 8] = 0x00;
			destinationArray[destinationIndex + 9] = 0x00;
			destinationArray[destinationIndex + 10] = 0x00;
			return true;
		}
		return reset(destinationArray, destinationIndex);
	}

	/// Galloping effect data generator.
	/// This is not used by libpad but is in the used effect block, it may be removed in a future DualSense firmware.
	/// @param destinationArray The uint8_t[] that receives the data.
	/// @param destinationIndex A 32-bit integer that represents the index in the destinationArray at which storing begins.
	/// @param start The starting zone of the trigger effect. Must be between 0 and 8 inclusive.
	/// @param end The ending zone of the trigger effect. Must be between <paramref name="start"/> + 1 and 9 inclusive.
	/// @param firstFoot Position of second foot in cycle. Must be between 0 and 6 inclusive.
	/// @param secondFoot Position of second foot in cycle. Must be between <paramref name="firstFoot"/> + 1 and 7 inclusive.
	/// @param frequency Frequency of the automatic cycling action in hertz.
	/// @returns The success of the effect write.
	static bool Galloping(uint8_t *destinationArray, int destinationIndex, uint8_t start, uint8_t end, uint8_t firstFoot, uint8_t secondFoot, uint8_t frequency)
	{
		if (start > 8)
			return false;
		if (end > 9)
			return false;
		if (start >= end)
			return false;
		if (secondFoot > 7)
			return false;
		if (firstFoot > 6)
			return false;
		if (firstFoot >= secondFoot)
			return false;
		if (frequency > 0)
		{
			uint16_t startAndStopZones = (uint16_t)((1 << start) | (1 << end));
			uint32_t timeAndRatio = (uint32_t)(((secondFoot & 0x07) << (3 * 0)) | ((firstFoot & 0x07) << (3 * 1)));

			destinationArray[destinationIndex + 0] = (uint8_t)TriggerEffectType::Galloping;
			destinationArray[destinationIndex + 1] = (uint8_t)((startAndStopZones >> 0) & 0xff);
			destinationArray[destinationIndex + 2] = (uint8_t)((startAndStopZones >> 8) & 0xff);
			destinationArray[destinationIndex + 3] = (uint8_t)((timeAndRatio >> 0) & 0xff);
			destinationArray[destinationIndex + 4] = frequency; // this is actually packed into 3 bits, but since it's only one why bother with the fancy code?
			destinationArray[destinationIndex + 5] = 0x00;
			destinationArray[destinationIndex + 6] = 0x00;
			destinationArray[destinationIndex + 7] = 0x00;
			destinationArray[destinationIndex + 8] = 0x00;
			destinationArray[destinationIndex + 9] = 0x00;
			destinationArray[destinationIndex + 10] = 0x00;
			return true;
		}
		return reset(destinationArray, destinationIndex);
	}

	/// Semi-automatic gun effect data generator.
	/// This is used by libpad and is expected to be present in future DualSense firmware.
	/// @param destinationArray The uint8_t[] that receives the data.
	/// @param destinationIndex A 32-bit integer that represents the index in the destinationArray at which storing begins.
	/// @param start The starting zone of the trigger effect. Must be between 2 and 7 inclusive.
	/// @param end The ending zone of the trigger effect. Must be between <paramref name="start"/> and 8 inclusive.
	/// @param force The force of the resistance. Must be between 0 and 8 inclusive.
	/// @returns The success of the effect write.
	static bool SemiAutomaticGun(uint8_t *destinationArray, int destinationIndex, uint8_t start, uint8_t end, uint8_t force)
	{
		if (start > 7 || start < 2)
			return false;
		if (end > 8)
			return false;
		if (end <= start)
			return false;
		if (force > 8)
			return false;
		if (force > 0)
		{
			uint16_t startAndStopZones = (uint16_t)((1 << start) | (1 << end));

			destinationArray[destinationIndex + 0] = (uint8_t)TriggerEffectType::SemiAutomaticGun;
			destinationArray[destinationIndex + 1] = (uint8_t)((startAndStopZones >> 0) & 0xff);
			destinationArray[destinationIndex + 2] = (uint8_t)((startAndStopZones >> 8) & 0xff);
			destinationArray[destinationIndex + 3] = (uint8_t)(force - 1); // this is actually packed into 3 bits, but since it's only one why bother with the fancy code?
			destinationArray[destinationIndex + 4] = 0x00;
			destinationArray[destinationIndex + 5] = 0x00;
			destinationArray[destinationIndex + 6] = 0x00;
			destinationArray[destinationIndex + 7] = 0x00;
			destinationArray[destinationIndex + 8] = 0x00;
			destinationArray[destinationIndex + 9] = 0x00;
			destinationArray[destinationIndex + 10] = 0x00;
			return true;
		}
		return reset(destinationArray, destinationIndex);
	}

	/// Automatic gun effect data generator.
	/// This is used by libpad and is expected to be present in future DualSense firmware.
	/// @param destinationArray The uint8_t[] that receives the data.
	/// @param destinationIndex A 32-bit integer that represents the index in the destinationArray at which storing begins.
	/// @param start The starting zone of the trigger effect. Must be between 0 and 9 inclusive.
	/// @param force Strength of the automatic cycling action. Must be between 0 and 8 inclusive.
	/// @param frequency Frequency of the automatic cycling action in hertz.
	/// @returns The success of the effect write.
	static bool AutomaticGun(uint8_t *destinationArray, int destinationIndex, uint8_t start, uint8_t strength, uint8_t frequency)
	{
		if (start > 9)
			return false;
		if (strength > 8)
			return false;
		if (strength > 0 && frequency > 0)
		{
			uint8_t strengthValue = (uint8_t)((strength - 1) & 0x07);
			uint32_t strengthZones = 0;
			uint16_t activeZones = 0;
			for (int i = start; i < 10; i++)
			{
				strengthZones |= (uint32_t)(strengthValue << (3 * i));
				activeZones |= (uint16_t)(1 << i);
			}

			destinationArray[destinationIndex + 0] = (uint8_t)TriggerEffectType::AutomaticGun;
			destinationArray[destinationIndex + 1] = (uint8_t)((activeZones >> 0) & 0xff);
			destinationArray[destinationIndex + 2] = (uint8_t)((activeZones >> 8) & 0xff);
			destinationArray[destinationIndex + 3] = (uint8_t)((strengthZones >> 0) & 0xff);
			destinationArray[destinationIndex + 4] = (uint8_t)((strengthZones >> 8) & 0xff);
			destinationArray[destinationIndex + 5] = (uint8_t)((strengthZones >> 16) & 0xff);
			destinationArray[destinationIndex + 6] = (uint8_t)((strengthZones >> 24) & 0xff);
			destinationArray[destinationIndex + 7] = 0x00; // (uint8_t)((strengthZones >> 32) & 0xff); // need 64bit for this, but we already have enough space
			destinationArray[destinationIndex + 8] = 0x00; // (uint8_t)((strengthZones >> 40) & 0xff); // need 64bit for this, but we already have enough space
			destinationArray[destinationIndex + 9] = frequency;
			destinationArray[destinationIndex + 10] = 0x00;
			return true;
		}
		return reset(destinationArray, destinationIndex);
	}

	/// Machine effect data generator.
	/// This is not used by libpad but is in the used effect block, it may be removed in a future DualSense firmware.
	/// @param destinationArray The uint8_t[] that receives the data.
	/// @param destinationIndex A 32-bit integer that represents the index in the destinationArray at which storing begins.
	/// @param start The starting zone of the trigger effect. Must be between 0 and 8 inclusive.
	/// @param end The starting zone of the trigger effect. Must be between <paramref name="start"/> and 9 inclusive.
	/// @param strengthA Primary force of cycling action. Must be between 0 and 7 inclusive.
	/// @param strengthB Secondary force of cycling action. Must be between 0 and 7 inclusive.
	/// @param frequency Frequency of the automatic cycling action in hertz.
	/// @param period Period of the oscillation between <paramref name="strengthA"/> and <paramref name="strengthB"/> in tenths of a second.
	/// @returns The success of the effect write.
	static bool Machine(uint8_t *destinationArray, int destinationIndex, uint8_t start, uint8_t end, uint8_t strengthA, uint8_t strengthB, uint8_t frequency, uint8_t period)
	{
		if (start > 8)
			return false;
		if (end > 9)
			return false;
		if (end <= start)
			return false;
		if (strengthA > 7)
			return false;
		if (strengthB > 7)
			return false;
		if (frequency > 0)
		{
			uint16_t startAndStopZones = (uint16_t)((1 << start) | (1 << end));
			uint32_t strengthPair = (uint32_t)(((strengthA & 0x07) << (3 * 0)) | ((strengthB & 0x07) << (3 * 1)));

			destinationArray[destinationIndex + 0] = (uint8_t)TriggerEffectType::Machine;
			destinationArray[destinationIndex + 1] = (uint8_t)((startAndStopZones >> 0) & 0xff);
			destinationArray[destinationIndex + 2] = (uint8_t)((startAndStopZones >> 8) & 0xff);
			destinationArray[destinationIndex + 3] = (uint8_t)((strengthPair >> 0) & 0xff);
			destinationArray[destinationIndex + 4] = frequency;
			destinationArray[destinationIndex + 5] = period;
			destinationArray[destinationIndex + 6] = 0x00;
			destinationArray[destinationIndex + 7] = 0x00;
			destinationArray[destinationIndex + 8] = 0x00;
			destinationArray[destinationIndex + 9] = 0x00;
			destinationArray[destinationIndex + 10] = 0x00;
			return true;
		}
		return reset(destinationArray, destinationIndex);
	}
};

} // namespace DualSense