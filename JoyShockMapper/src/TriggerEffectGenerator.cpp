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
 * found at this location https://gist.github.com/Nielk1/6d54cc2c00d2201ccb8c2720ad7538db under
 * the same MIT license.
 */
 
#include "TriggerEffectGenerator.h"
#include <cstdint>

namespace ExtendInput
{
namespace DataTools
{
namespace DualSense
{

bool TriggerEffectGenerator::SimpleResistance(byte *destinationArray, int destinationIndex, byte start, byte force)
{
	destinationArray[destinationIndex + 0] = (byte)TriggerEffectType::SimpleResistance;
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

bool TriggerEffectGenerator::SimpleSemiAutomaticGun(byte *destinationArray, int destinationIndex, byte start, byte end, byte force)
{
	destinationArray[destinationIndex + 0] = (byte)TriggerEffectType::SimpleSemiAutomaticGun;
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

bool TriggerEffectGenerator::Reset(byte *destinationArray, int destinationIndex)
{
	destinationArray[destinationIndex + 0] = (byte)TriggerEffectType::Reset;
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

bool TriggerEffectGenerator::SimpleAutomaticGun(byte *destinationArray, int destinationIndex, byte start, byte strength, byte frequency)
{
	if (frequency > 0 && strength > 0)
	{
		destinationArray[destinationIndex + 0] = (byte)TriggerEffectType::SimpleAutomaticGun;
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
	return Reset(destinationArray, destinationIndex);
}

bool TriggerEffectGenerator::LimitedResistance(byte *destinationArray, int destinationIndex, byte start, byte force)
{
	if (force > 10)
		return false;
	if (force > 0)
	{
		destinationArray[destinationIndex + 0] = (byte)TriggerEffectType::LimitedResistance;
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
	return Reset(destinationArray, destinationIndex);
}

bool TriggerEffectGenerator::LimitedSemiAutomaticGun(byte *destinationArray, int destinationIndex, byte start, byte end, byte force)
{
	if (start < 0x10)
		return false;
	if (end < start || (start + 100) < end)
		return false;
	if (force > 10)
		return false;
	if (force > 0)
	{
		destinationArray[destinationIndex + 0] = (byte)TriggerEffectType::LimitedSemiAutomaticGun;
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
	return Reset(destinationArray, destinationIndex);
}

bool TriggerEffectGenerator::Resistance(byte *destinationArray, int destinationIndex, byte start, byte force)
{
	if (start > 9)
		return false;
	if (force > 8)
		return false;
	if (force > 0)
	{
		byte forceValue = (byte)((force - 1) & 0x07);
		uint32_t forceZones = 0;
		uint16_t activeZones = 0;
		for (int i = start; i < 10; i++)
		{
			forceZones |= (uint32_t)(forceValue << (3 * i));
			activeZones |= (uint16_t)(1 << i);
		}

		destinationArray[destinationIndex + 0] = (byte)TriggerEffectType::Resistance;
		destinationArray[destinationIndex + 1] = (byte)((activeZones >> 0) & 0xff);
		destinationArray[destinationIndex + 2] = (byte)((activeZones >> 8) & 0xff);
		destinationArray[destinationIndex + 3] = (byte)((forceZones >> 0) & 0xff);
		destinationArray[destinationIndex + 4] = (byte)((forceZones >> 8) & 0xff);
		destinationArray[destinationIndex + 5] = (byte)((forceZones >> 16) & 0xff);
		destinationArray[destinationIndex + 6] = (byte)((forceZones >> 24) & 0xff);
		destinationArray[destinationIndex + 7] = 0x00; // (byte)((forceZones >> 32) & 0xff); // need 64bit for this, but we already have enough space
		destinationArray[destinationIndex + 8] = 0x00; // (byte)((forceZones >> 40) & 0xff); // need 64bit for this, but we already have enough space
		destinationArray[destinationIndex + 9] = 0x00;
		destinationArray[destinationIndex + 10] = 0x00;
		return true;
	}
	return Reset(destinationArray, destinationIndex);
}

bool TriggerEffectGenerator::Bow(byte *destinationArray, int destinationIndex, byte start, byte end, byte force, byte snapForce)
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
		uint32_t forcePair = (uint32_t)((((force - 1) & 0x07) << (3 * 0))
								  | (((snapForce - 1) & 0x07) << (3 * 1)));

		destinationArray[destinationIndex + 0] = (byte)TriggerEffectType::Bow;
		destinationArray[destinationIndex + 1] = (byte)((startAndStopZones >> 0) & 0xff);
		destinationArray[destinationIndex + 2] = (byte)((startAndStopZones >> 8) & 0xff);
		destinationArray[destinationIndex + 3] = (byte)((forcePair >> 0) & 0xff);
		destinationArray[destinationIndex + 4] = (byte)((forcePair >> 8) & 0xff);
		destinationArray[destinationIndex + 5] = 0x00;
		destinationArray[destinationIndex + 6] = 0x00;
		destinationArray[destinationIndex + 7] = 0x00;
		destinationArray[destinationIndex + 8] = 0x00;
		destinationArray[destinationIndex + 9] = 0x00;
		destinationArray[destinationIndex + 10] = 0x00;
		return true;
	}
	return Reset(destinationArray, destinationIndex);
}

bool TriggerEffectGenerator::Galloping(byte *destinationArray, int destinationIndex, byte start, byte end, byte firstFoot, byte secondFoot, byte frequency)
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
		uint32_t timeAndRatio = (uint32_t)(((secondFoot & 0x07) << (3 * 0))
									 | ((firstFoot  & 0x07) << (3 * 1)));

		destinationArray[destinationIndex + 0] = (byte)TriggerEffectType::Galloping;
		destinationArray[destinationIndex + 1] = (byte)((startAndStopZones >> 0) & 0xff);
		destinationArray[destinationIndex + 2] = (byte)((startAndStopZones >> 8) & 0xff);
		destinationArray[destinationIndex + 3] = (byte)((timeAndRatio >> 0) & 0xff);
		destinationArray[destinationIndex + 4] = frequency; // this is actually packed into 3 bits, but since it's only one why bother with the fancy code?
		destinationArray[destinationIndex + 5] = 0x00;
		destinationArray[destinationIndex + 6] = 0x00;
		destinationArray[destinationIndex + 7] = 0x00;
		destinationArray[destinationIndex + 8] = 0x00;
		destinationArray[destinationIndex + 9] = 0x00;
		destinationArray[destinationIndex + 10] = 0x00;
		return true;
	}
	return Reset(destinationArray, destinationIndex);
}

bool TriggerEffectGenerator::SemiAutomaticGun(byte *destinationArray, int destinationIndex, byte start, byte end, byte force)
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

		destinationArray[destinationIndex + 0] = (byte)TriggerEffectType::SemiAutomaticGun;
		destinationArray[destinationIndex + 1] = (byte)((startAndStopZones >> 0) & 0xff);
		destinationArray[destinationIndex + 2] = (byte)((startAndStopZones >> 8) & 0xff);
		destinationArray[destinationIndex + 3] = (byte)(force - 1); // this is actually packed into 3 bits, but since it's only one why bother with the fancy code?
		destinationArray[destinationIndex + 4] = 0x00;
		destinationArray[destinationIndex + 5] = 0x00;
		destinationArray[destinationIndex + 6] = 0x00;
		destinationArray[destinationIndex + 7] = 0x00;
		destinationArray[destinationIndex + 8] = 0x00;
		destinationArray[destinationIndex + 9] = 0x00;
		destinationArray[destinationIndex + 10] = 0x00;
		return true;
	}
	return Reset(destinationArray, destinationIndex);
}

bool TriggerEffectGenerator::AutomaticGun(byte *destinationArray, int destinationIndex, byte start, byte strength, byte frequency)
{
	if (start > 9)
		return false;
	if (strength > 8)
		return false;
	if (strength > 0 && frequency > 0)
	{
		byte strengthValue = (byte)((strength - 1) & 0x07);
		uint32_t strengthZones = 0;
		uint16_t activeZones = 0;
		for (int i = start; i < 10; i++)
		{
			strengthZones |= (uint32_t)(strengthValue << (3 * i));
			activeZones |= (uint16_t)(1 << i);
		}

		destinationArray[destinationIndex + 0] = (byte)TriggerEffectType::AutomaticGun;
		destinationArray[destinationIndex + 1] = (byte)((activeZones >> 0) & 0xff);
		destinationArray[destinationIndex + 2] = (byte)((activeZones >> 8) & 0xff);
		destinationArray[destinationIndex + 3] = (byte)((strengthZones >> 0) & 0xff);
		destinationArray[destinationIndex + 4] = (byte)((strengthZones >> 8) & 0xff);
		destinationArray[destinationIndex + 5] = (byte)((strengthZones >> 16) & 0xff);
		destinationArray[destinationIndex + 6] = (byte)((strengthZones >> 24) & 0xff);
		destinationArray[destinationIndex + 7] = 0x00; // (byte)((strengthZones >> 32) & 0xff); // need 64bit for this, but we already have enough space
		destinationArray[destinationIndex + 8] = 0x00; // (byte)((strengthZones >> 40) & 0xff); // need 64bit for this, but we already have enough space
		destinationArray[destinationIndex + 9] = frequency;
		destinationArray[destinationIndex + 10] = 0x00;
		return true;
	}
	return Reset(destinationArray, destinationIndex);
}

bool TriggerEffectGenerator::Machine(byte *destinationArray, int destinationIndex, byte start, byte end, byte strengthA, byte strengthB, byte frequency, byte period)
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
		uint32_t strengthPair = (uint32_t)(((strengthA & 0x07) << (3 * 0))
									 | ((strengthB & 0x07) << (3 * 1)));

		destinationArray[destinationIndex + 0] = (byte)TriggerEffectType::Machine;
		destinationArray[destinationIndex + 1] = (byte)((startAndStopZones >> 0) & 0xff);
		destinationArray[destinationIndex + 2] = (byte)((startAndStopZones >> 8) & 0xff);
		destinationArray[destinationIndex + 3] = (byte)((strengthPair >> 0) & 0xff);
		destinationArray[destinationIndex + 4] = frequency;
		destinationArray[destinationIndex + 5] = period;
		destinationArray[destinationIndex + 6] = 0x00;
		destinationArray[destinationIndex + 7] = 0x00;
		destinationArray[destinationIndex + 8] = 0x00;
		destinationArray[destinationIndex + 9] = 0x00;
		destinationArray[destinationIndex + 10] = 0x00;
		return true;
	}
	return Reset(destinationArray, destinationIndex);
}

}
}
}