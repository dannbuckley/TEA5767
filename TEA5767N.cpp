/*
   Copyright 2014 Marcos R. Oliveira

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <Arduino.h>
#include <Wire.h>
#include <TEA5767N.h>

TEA5767N::TEA5767N()
{
	Wire.begin();
	initializeTransmissionData();

	muted = false;
	mutedLeft = false;
	mutedRight = false;
	softMuted = false;
	highCutControl = false;
	stereoNoiseCancelling = false;
	standby = false;
	forcedMono = false;
}

void TEA5767N::initializeTransmissionData()
{
	transmission_data[FIRST_DATA] = 0; //MUTE: 0 - not muted
																		 //SEARCH MODE: 0 - not in search mode

	transmission_data[SECOND_DATA] = 0; //No frequency defined yet

	transmission_data[THIRD_DATA] = 0xB0; //10110000
																				//SUD: 1 - search up
																				//SSL[1:0]: 01 - low; level ADC output = 5
																				//HLSI: 1 - high side LO injection
																				//MS: 0 - stereo ON
																				//MR: 0 - right audio channel is not muted
																				//ML: 0 - left audio channel is not muted
																				//SWP1: 0 - port 1 is LOW

	transmission_data[FOURTH_DATA] = 0x10; //00010000
																				 //SWP2: 0 - port 2 is LOW
																				 //STBY: 0 - not in Standby mode
																				 //BL: 0 - US/Europe FM band
																				 //XTAL: 1 - 32.768 kHz
																				 //SMUTE: 0 - soft mute is OFF
																				 //HCC: 0 - high cut control is OFF
																				 //SNC: 0 - stereo noise cancelling is OFF
																				 //SI: 0 - pin SWPORT1 is software programmable port 1

	transmission_data[FIFTH_DATA] = 0x00; //PLLREF: 0 - the 6.5 MHz reference frequency for the PLL is disabled
																				//DTC: 0 - the de-emphasis time constant is 50 ms
}

void TEA5767N::transmitData()
{
	Wire.beginTransmission(TEA5767_I2C_ADDRESS);
	for (int i = 0; i < 5; i++)
	{
		Wire.write(transmission_data[i]);
	}
	Wire.endTransmission();
	delay(100);
}

void TEA5767N::readStatus()
{
	Wire.requestFrom(TEA5767_I2C_ADDRESS, 5);
	if (Wire.available())
	{
		for (int i = 0; i < 5; i++)
		{
			reception_data[i] = Wire.read();
		}
	}
	delay(100);
}

void TEA5767N::calculateOptimalLOInjection(float freq)
{
	byte signalHigh;
	byte signalLow;

	setHighSideLOInjection();
	setFrequency((float)(freq + 0.45));
	signalHigh = getSignalLevel();

	setLowSideLOInjection();
	setFrequency((float)(freq - 0.45));
	signalLow = getSignalLevel();

	hiInjection = (signalHigh < signalLow) ? 1 : 0;
}

void TEA5767N::setFrequency(float _frequency)
{
	frequency = _frequency;
	unsigned int frequencyW;

	if (hiInjection)
	{
		setHighSideLOInjection();
		frequencyW = 4 * ((frequency * 1000000) + 225000) / 32768;
	}
	else
	{
		setLowSideLOInjection();
		frequencyW = 4 * ((frequency * 1000000) - 225000) / 32768;
	}

	transmission_data[FIRST_DATA] = ((transmission_data[FIRST_DATA] & 0xC0) | ((frequencyW >> 8) & 0x3F));
	transmission_data[SECOND_DATA] = frequencyW & 0XFF;
}

void TEA5767N::toggleMute()
{
	muted = !muted;

	if (muted)
	{
		transmission_data[FIRST_DATA] |= 0b10000000;
	}
	else
	{
		transmission_data[FIRST_DATA] &= 0b01111111;
	}

	transmitData();
}

void TEA5767N::toggleMuteLeft()
{
	mutedLeft = !mutedLeft;

	if (mutedRight)
	{
		transmission_data[THIRD_DATA] |= 0b00000010;
	}
	else
	{
		transmission_data[THIRD_DATA] &= 0b11111101;
	}

	transmitData();
}

void TEA5767N::toggleMuteRight()
{
	mutedRight = !mutedRight;

	if (mutedRight)
	{
		transmission_data[THIRD_DATA] |= 0b00000100;
	}
	else
	{
		transmission_data[THIRD_DATA] &= 0b11111011;
	}

	transmitData();
}

void TEA5767N::toggleSoftMute()
{
	softMuted = !softMuted;

	if (softMuted)
	{
		transmission_data[FOURTH_DATA] |= 0b00001000;
	}
	else
	{
		transmission_data[FOURTH_DATA] &= 0b11110111;
	}

	transmitData();
}

boolean TEA5767N::isMuted()
{
	return muted;
}

boolean TEA5767N::isMutedLeft()
{
	return mutedLeft;
}

boolean TEA5767N::isMutedRight()
{
	return mutedRight;
}

boolean TEA5767N::isSoftMuted()
{
	return softMuted;
}

void TEA5767N::selectFrequency(float frequency, boolean muteDuringUpdate)
{
	if (muteDuringUpdate)
	{
		muted = false;
		toggleMute();
	}

	calculateOptimalLOInjection(frequency);
	setFrequency(frequency);

	if (muteDuringUpdate)
	{
		toggleMute();
	}
}

void TEA5767N::selectChannel(int channel, boolean muteDuringUpdate)
{
	if (muteDuringUpdate)
	{
		muted = false;
		toggleMute();
	}

	float frequency = 87.9 + ((channel - 200) * 0.2);
	selectFrequency(frequency, false);

	if (muteDuringUpdate)
	{
		toggleMute();
	}
}

void TEA5767N::loadFrequency()
{
	readStatus();

	transmission_data[FIRST_DATA] = (transmission_data[FIRST_DATA] & 0xC0) | (reception_data[FIRST_DATA] & 0x3F);
	transmission_data[SECOND_DATA] = reception_data[SECOND_DATA];
}

float TEA5767N::getFrequencyInMHz(unsigned int frequencyW)
{
	if (hiInjection)
	{
		return (((frequencyW / 4.0) * 32768.0) - 225000.0) / 1000000.0;
	}
	else
	{
		return (((frequencyW / 4.0) * 32768.0) + 225000.0) / 1000000.0;
	}
}

float TEA5767N::readFrequencyInMHz()
{
	loadFrequency();

	unsigned int frequencyW = (((reception_data[FIRST_DATA] & 0x3F) * 256) + reception_data[SECOND_DATA]);
	return getFrequencyInMHz(frequencyW);
}

void TEA5767N::setSearchUp()
{
	transmission_data[THIRD_DATA] |= 0b10000000;
}

void TEA5767N::setSearchDown()
{
	transmission_data[THIRD_DATA] &= 0b01111111;
}

void TEA5767N::setSearchLowStopLevel()
{
	transmission_data[THIRD_DATA] &= 0b10011111;
	transmission_data[THIRD_DATA] |= (LOW_STOP_LEVEL << 5);
}

void TEA5767N::setSearchMidStopLevel()
{
	transmission_data[THIRD_DATA] &= 0b10011111;
	transmission_data[THIRD_DATA] |= (MID_STOP_LEVEL << 5);
}

void TEA5767N::setSearchHighStopLevel()
{
	transmission_data[THIRD_DATA] &= 0b10011111;
	transmission_data[THIRD_DATA] |= (HIGH_STOP_LEVEL << 5);
}

void TEA5767N::setHighSideLOInjection()
{
	transmission_data[THIRD_DATA] |= 0b00010000;
}

void TEA5767N::setLowSideLOInjection()
{
	transmission_data[THIRD_DATA] &= 0b11101111;
}

byte TEA5767N::searchNext(boolean muteDuringSearch)
{
	byte bandLimitReached;

	if (muteDuringSearch)
	{
		muted = false;
		toggleMute();
	}

	if (isSearchUp())
	{
		selectFrequency(readFrequencyInMHz() + 0.1, false);
	}
	else
	{
		selectFrequency(readFrequencyInMHz() - 0.1, false);
	}

	transmission_data[FIRST_DATA] |= 0b01000000;
	transmitData();

	while (!isReady())
	{
	}

	bandLimitReached = isBandLimitReached();
	loadFrequency();

	transmission_data[FIRST_DATA] &= 0b10111111;
	transmitData();

	if (muteDuringSearch)
	{
		toggleMute();
	}

	return bandLimitReached;
}

byte TEA5767N::searchFrom(float frequency, boolean muteDuringSearch)
{
	selectFrequency(frequency, false);
	return searchNext(muteDuringSearch);
}

byte TEA5767N::searchFromBeginning(boolean muteDuringSearch)
{
	byte bandLimitReached;
	setSearchUp();

	if (muteDuringSearch)
	{
		muted = false;
		toggleMute();
	}

	bandLimitReached = searchFrom(87.0, false);

	if (muteDuringSearch)
	{
		toggleMute();
	}

	return bandLimitReached;
}

byte TEA5767N::searchFromEnd(boolean muteDuringSearch)
{
	byte bandLimitReached;
	setSearchDown();

	if (muteDuringSearch)
	{
		muted = false;
		toggleMute();
	}

	bandLimitReached = searchFrom(108.0, false);

	if (muteDuringSearch)
	{
		toggleMute();
	}

	return bandLimitReached;
}

byte TEA5767N::getSignalLevel()
{
	// Necessary before read status
	transmitData();
	// Read updated status
	readStatus();
	return reception_data[FOURTH_DATA] >> 4;
}

byte TEA5767N::isStereo()
{
	readStatus();
	return reception_data[THIRD_DATA] >> 7;
}

byte TEA5767N::isReady()
{
	readStatus();
	return reception_data[FIRST_DATA] >> 7;
}

byte TEA5767N::isBandLimitReached()
{
	readStatus();
	return (reception_data[FIRST_DATA] >> 6) & 1;
}

byte TEA5767N::isSearchUp()
{
	return (transmission_data[THIRD_DATA] & 0b10000000) != 0;
}

byte TEA5767N::isSearchDown()
{
	return (transmission_data[THIRD_DATA] & 0b10000000) == 0;
}

boolean TEA5767N::isStandBy()
{
	readStatus();
	return (transmission_data[FOURTH_DATA] & 0b01000000) != 0;
}

void TEA5767N::toggleForcedMono()
{
	forcedMono = !forcedMono;

	if (forcedMono)
	{
		transmission_data[THIRD_DATA] |= 0b00001000;
	}
	else
	{
		transmission_data[THIRD_DATA] &= 0b11110111;
	}

	transmitData();
}

void TEA5767N::toggleStandby()
{
	standby = !standby;

	if (standby)
	{
		transmission_data[FOURTH_DATA] |= 0b01000000;
	}
	else
	{
		transmission_data[FOURTH_DATA] &= 0b10111111;
	}

	transmitData();
}

void TEA5767N::toggleHighCutControl()
{
	highCutControl = !highCutControl;

	if (highCutControl)
	{
		transmission_data[FOURTH_DATA] |= 0b00000100;
	}
	else
	{
		transmission_data[FOURTH_DATA] &= 0b11111011;
	}

	transmitData();
}

void TEA5767N::toggleStereoNoiseCancelling()
{
	stereoNoiseCancelling = !stereoNoiseCancelling;

	if (stereoNoiseCancelling)
	{
		transmission_data[FOURTH_DATA] |= 0b00000010;
	}
	else
	{
		transmission_data[FOURTH_DATA] &= 0b11111101;
	}

	transmitData();
}