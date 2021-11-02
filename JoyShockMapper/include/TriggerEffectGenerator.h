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

#pragma once

namespace ExtendInput
{
namespace DataTools
{
namespace DualSense
{
	
using byte = unsigned char;

enum class TriggerEffectType : byte
{
	SimpleResistance = 0x01,       // 00 00 0 001 // Suggest don't use
	SimpleSemiAutomaticGun = 0x02, // 00 00 0 100 // Suggest don't use
	Reset = 0x05,                  // 00 00 0 101 // Safe to Use, part of libpad
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

/// DualSense controller trigger effect generators.
/// Revision: 1
class TriggerEffectGenerator
{
public:

	/// Simplistic resistance effect data generator.
	/// This is not used by libpad and may be removed in a future DualSense firmware.
	/// @note Use <see cref="Resistance(byte[], int, byte, byte)"/> instead.
	/// @param destinationArray The byte[] that receives the data.
	/// @param destinationIndex A 32-bit integer that represents the index in the destinationArray at which storing begins.
	/// @param start The starting zone of the trigger effect.
	/// @param force The force of the resistance.
	/// @returns The success of the effect write.
	static bool SimpleResistance(byte *destinationArray, int destinationIndex, byte start, byte force);

	/// Simplistic semi-automatic gun effect data generator.
	/// This is not used by libpad and may be removed in a future DualSense firmware.
	/// @note Use <see cref="SemiAutomaticGun(byte[], int, byte, byte, byte)"/> instead.
	/// @param destinationArray The byte[] that receives the data.
	/// @param destinationIndex A 32-bit integer that represents the index in the destinationArray at which storing begins.
	/// @param start The starting zone of the trigger effect.
	/// @param end The ending zone of the trigger effect.
	/// @param force The force of the resistance.
	/// @returns The success of the effect write.
	static bool SimpleSemiAutomaticGun(byte *destinationArray, int destinationIndex, byte start, byte end, byte force);

	/// Reset effect data generator.
	/// This is used by libpad and is expected to be present in future DualSense firmware.
	/// @param destinationArray The byte[] that receives the data.
	/// @param destinationIndex A 32-bit integer that represents the index in the destinationArray at which storing begins.
	/// @returns The success of the effect write.
	static bool Reset(byte *destinationArray, int destinationIndex);

	/// Simplistic automatic gun effect data generator.
	/// This is not used by libpad and may be removed in a future DualSense firmware.
	/// @note Use <see cref="AutomaticGun(byte[], int, byte, byte, byte)"/> instead.
	/// @param destinationArray The byte[] that receives the data.
	/// @param destinationIndex A 32-bit integer that represents the index in the destinationArray at which storing begins.
	/// @param start The starting zone of the trigger effect.
	/// @param force Strength of the automatic cycling action.
	/// @param frequency Frequency of the automatic cycling action in hertz.
	/// @returns The success of the effect write.
	static bool SimpleAutomaticGun(byte *destinationArray, int destinationIndex, byte start, byte strength, byte frequency);

	/// Simplistic resistance effect data generator with stricter paramater limits.
	/// This is not used by libpad and may be removed in a future DualSense firmware.
	/// @note <see cref="Resistance(byte[], int, byte, byte)"/> instead.
	/// @param destinationArray The byte[] that receives the data.
	/// @param destinationIndex A 32-bit integer that represents the index in the destinationArray at which storing begins.
	/// @param start The starting zone of the trigger effect.
	/// @param force The force of the resistance. Must be between 0 and 10 inclusive.
	/// @returns The success of the effect write.
	static bool LimitedResistance(byte *destinationArray, int destinationIndex, byte start, byte force);

	/// Simplistic semi-automatic gun effect data generator with stricter paramater limits.
	/// This is not used by libpad and may be removed in a future DualSense firmware.
	/// @note <see cref="SemiAutomaticGun(byte[], int, byte, byte, byte)"/> instead.
	/// @param destinationArray The byte[] that receives the data.
	/// @param destinationIndex A 32-bit integer that represents the index in the destinationArray at which storing begins.
	/// @param start The starting zone of the trigger effect. Must be 16 or higher.
	/// @param end The ending zone of the trigger effect. Must be between <paramref name="start"/> and <paramref name="start"/>+100 inclusive.
	/// @param force The force of the resistance. Must be between 0 and 10 inclusive.
	/// @returns The success of the effect write.
	static bool LimitedSemiAutomaticGun(byte *destinationArray, int destinationIndex, byte start, byte end, byte force);

	/// Resistance effect data generator.
	/// This is used by libpad and is expected to be present in future DualSense firmware.
	/// @param destinationArray The byte[] that receives the data.
	/// @param destinationIndex A 32-bit integer that represents the index in the destinationArray at which storing begins.
	/// @param start The starting zone of the trigger effect. Must be between 0 and 9 inclusive.
	/// @param force The force of the resistance. Must be between 0 and 8 inclusive.
	/// @returns The success of the effect write.
	static bool Resistance(byte *destinationArray, int destinationIndex, byte start, byte force);

	/// Bow effect data generator.
	/// This is not used by libpad but is in the used effect block, it may be removed in a future DualSense firmware.
	/// @param destinationArray The byte[] that receives the data.
	/// @param destinationIndex A 32-bit integer that represents the index in the destinationArray at which storing begins.
	/// @param start The starting zone of the trigger effect. Must be between 0 and 8 inclusive.
	/// @param end The ending zone of the trigger effect. Must be between <paramref name="start"/> + 1 and 8 inclusive.
	/// @param force The force of the resistance. Must be between 0 and 8 inclusive.
	/// @param snapForce The force of the snap-back. Must be between 0 and 8 inclusive.
	/// @returns The success of the effect write.
	static bool Bow(byte *destinationArray, int destinationIndex, byte start, byte end, byte force, byte snapForce);

	/// Galloping effect data generator.
	/// This is not used by libpad but is in the used effect block, it may be removed in a future DualSense firmware.
	/// @param destinationArray The byte[] that receives the data.
	/// @param destinationIndex A 32-bit integer that represents the index in the destinationArray at which storing begins.
	/// @param start The starting zone of the trigger effect. Must be between 0 and 8 inclusive.
	/// @param end The ending zone of the trigger effect. Must be between <paramref name="start"/> + 1 and 9 inclusive.
	/// @param firstFoot Position of second foot in cycle. Must be between 0 and 6 inclusive.
	/// @param secondFoot Position of second foot in cycle. Must be between <paramref name="firstFoot"/> + 1 and 7 inclusive.
	/// @param frequency Frequency of the automatic cycling action in hertz.
	/// @returns The success of the effect write.
	static bool Galloping(byte *destinationArray, int destinationIndex, byte start, byte end, byte firstFoot, byte secondFoot, byte frequency);

	/// Semi-automatic gun effect data generator.
	/// This is used by libpad and is expected to be present in future DualSense firmware.
	/// @param destinationArray The byte[] that receives the data.
	/// @param destinationIndex A 32-bit integer that represents the index in the destinationArray at which storing begins.
	/// @param start The starting zone of the trigger effect. Must be between 2 and 7 inclusive.
	/// @param end The ending zone of the trigger effect. Must be between <paramref name="start"/> and 8 inclusive.
	/// @param force The force of the resistance. Must be between 0 and 8 inclusive.
	/// @returns The success of the effect write.
	static bool SemiAutomaticGun(byte *destinationArray, int destinationIndex, byte start, byte end, byte force);

	/// Automatic gun effect data generator.
	/// This is used by libpad and is expected to be present in future DualSense firmware.
	/// @param destinationArray The byte[] that receives the data.
	/// @param destinationIndex A 32-bit integer that represents the index in the destinationArray at which storing begins.
	/// @param start The starting zone of the trigger effect. Must be between 0 and 9 inclusive.
	/// @param force Strength of the automatic cycling action. Must be between 0 and 8 inclusive.
	/// @param frequency Frequency of the automatic cycling action in hertz.
	/// @returns The success of the effect write.
	static bool AutomaticGun(byte *destinationArray, int destinationIndex, byte start, byte strength, byte frequency);

	/// Machine effect data generator.
	/// This is not used by libpad but is in the used effect block, it may be removed in a future DualSense firmware.
	/// @param destinationArray The byte[] that receives the data.
	/// @param destinationIndex A 32-bit integer that represents the index in the destinationArray at which storing begins.
	/// @param start The starting zone of the trigger effect. Must be between 0 and 8 inclusive.
	/// @param end The starting zone of the trigger effect. Must be between <paramref name="start"/> and 9 inclusive.
	/// @param strengthA Primary force of cycling action. Must be between 0 and 7 inclusive.
	/// @param strengthB Secondary force of cycling action. Must be between 0 and 7 inclusive.
	/// @param frequency Frequency of the automatic cycling action in hertz.
	/// @param period Period of the oscillation between <paramref name="strengthA"/> and <paramref name="strengthB"/> in tenths of a second.
	/// @returns The success of the effect write.
	static bool Machine(byte *destinationArray, int destinationIndex, byte start, byte end, byte strengthA, byte strengthB, byte frequency, byte period);
};

}
}
}