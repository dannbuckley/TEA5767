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

#ifndef TEA5767N_H
#define TEA5767N_H

//Note that not all of these constants are being used. They are here for
//convenience, though.
#define TEA5767_I2C_ADDRESS 0x60

#define FIRST_DATA 0
#define SECOND_DATA 1
#define THIRD_DATA 2
#define FOURTH_DATA 3
#define FIFTH_DATA 4

#define LOW_STOP_LEVEL 1
#define MID_STOP_LEVEL 2
#define HIGH_STOP_LEVEL 3

#define HIGH_SIDE_INJECTION 1
#define LOW_SIDE_INJECTION 0

#define STEREO_ON 0
#define STEREO_OFF 1

#define MUTE_RIGHT_ON 1
#define MUTE_RIGHT_OFF 0
#define MUTE_LEFT_ON 1
#define MUTE_LEFT_OFF 0

#define SWP1_HIGH 1
#define SWP1_LOW 0
#define SWP2_HIGH 1
#define SWP2_LOW 0

#define STBY_ON 1
#define STBY_OFF 0

#define JAPANESE_FM_BAND 1
#define US_EUROPE_FM_BAND 0

#define SOFT_MUTE_ON 1
#define SOFT_MUTE_OFF 0

#define HIGH_CUT_CONTROL_ON 1
#define HIGH_CUT_CONTROL_OFF 0

#define STEREO_NOISE_CANCELLING_ON 1
#define STEREO_NOISE_CANCELLING_OFF 0

#define SEARCH_INDICATOR_ON 1
#define SEARCH_INDICATOR_OFF 0

class TEA5767N
{
private:
  float frequency;
  byte hiInjection;
  byte frequencyH;
  byte frequencyL;

  /*
		*
		* Transmission data layout:
		*
		* 	FIRST_DATA:
		* 		{bit 7}   MUTE: if MUTE = 1 then L and R audio are muted; if MUTE = 0 then L and R audio are not muted
		* 		{bit 6}   SM (Search Mode): if SM = 1 then in search mode; if SM = 0 then not in search mode
		* 		{bit 5:0} PLL[13:8]: setting of synthesizer programmable counter for search or preset
		*
		* 	SECOND_DATA:
		* 		{bit 7:0} PLL[7:0]: setting of synthesizer programmable counter for search or preset
		*
		* 	THIRD_DATA:
		* 		{bit 7}   SUD (Search Up/Down): if SUD = 1 then search up; if SUD = 0 then search down
		* 		{bit 6:5} SSL[1:0] (Search Stop Level):
		* 			[SSL1][SSL0] - search stop level
		* 			00 - not allowed in search mode
		* 			01 - low; level ADC output = 5
		* 			10 - mid; level ADC output = 7
		* 			11 - high; level ADC output = 10
		* 		{bit 4}   HLSI (High/Low Side Injection): if HLSI = 1 then high side LO injection; if HLSI = 0 then low side LO injection
		* 		{bit 3}   MS (Mono to Stereo): if MS = 1 then forced mono; if MS = 0 then stereo ON
		* 		{bit 2}   MR (Mute Right): if MR = 1 then the right audio channel is muted and forced mono; if MR = 0 then the right audio channel is not muted
		* 		{bit 1}   ML (Mute Left): if ML = 1 then the left audio channel is muted and forced mono; if ML = 0 then the left audio channel is not muted
		* 		{bit 0}   SWP1 (Software programmable port 1): if SWP1 = 1 then port 1 is HIGH; if SWP1 = 0 then port 1 is LOW
		*
		* 	FOURTH_DATA:
		* 		{bit 7} SWP2 (Software programmable port 2): if SWP2 = 1 then port 2 is HIGH; if SWP2 = 0 then port 2 is LOW
		* 		{bit 6} STBY (Standby): if STBY = 1 then in Standby mode; if STBY = 0 then not in Standby mode
		* 		{bit 5} BL (Band Limits): if BL = 1 then Japanese FM band; if BL = 0 then US/Europe FM band
		* 		{bit 4} XTAL (Clock frequency):
		* 			[PLLREF][XTAL] - Clock frequency
		* 			00 - 13 MHz
		* 			01 - 32.678 kHz
		* 			10 - 6.5MHz
		* 			11 - not allowed
		* 		{bit 3} SMUTE (Soft Mute): if SMUTE = 1 then soft mute is ON; if SMUTE = 0 then soft mute is OFF
		* 		{bit 2} HCC (High Cut Control): if HCC = 1 then high cut control is ON; if HCC = 0 then high cut control is OFF
		* 		{bit 1} SNC (Stereo Noise Cancelling): if SNC = 1 then stereo noise cancelling is ON; if SNC = 0 then stereo noise cancelling is OFF
		* 		{bit 0} SI (Search Indicator): if SI = 1 then pin SWPORT1 is output for the ready flag; if SI = 0 then pin SWPORT1 is software programmable port 1
		*
		* 	FIFTH_DATA:
		* 		{bit 7}   PLLREF: if PLLREF = 1 then the 6.5 MHz reference frequency for the PLL is enabled; if PLLREF = 0 then the 6.5 MHz reference frequency for the PLL is disabled; see table at XTAL
		* 		{bit 6}   DTC: if DTC = 1 then the de-emphasis time constant is 75 microseconds; if DTC = 0 then the de-emphasis time constant is 50 microseconds
		* 		{bit 5:0} not used; position is don’t care
		*
		*/
  byte transmission_data[5];

  /*
		*
		* Reception data layout:
		*
		* 	FIRST_DATA:
		* 		{bit 7}   RF (Ready Flag): if RF = 1 then a station has been found or the band limit has been reached; if RF = 0 then no station has been found
		* 		{bit 6}   BLF (Band Limit Flag): if BLF = 1 then the band limit has been reached; if BLF = 0 then the band limit has not been reached
		* 		{bit 5:0} PLL[13:8]: setting of synthesizer programmable counter after search or preset
		*
		* 	SECOND_DATA:
		* 		{bit 7:0} PLL[7:0]: setting of synthesizer programmable counter after search or preset
		*
		* 	THIRD_DATA:
		* 		{bit 7}   STEREO (Stereo indication): if STEREO = 1 then stereo reception; if STEREO = 0 then mono reception
		* 		{bit 6:0} PLL[13:8]: IF counter result
		*
		* 	FOURTH_DATA:
		* 		{bit 7:4} LEV[3:0]: level ADC output
		* 		{bit 3:1} CI (Chip Identification): these bits have to be set to logic 0
		* 		{bit 0}   this bit is internally set to logic 0
		*
		* 	FIFTH_DATA:
		* 		{bit 7:0} reserved for future extensions; these bits are internally set to logic 0
		*
		*/
  byte reception_data[5];

  boolean muted;
  boolean mutedLeft;
  boolean mutedRight;
  boolean softMuted;
  boolean highCutControl;
  boolean stereoNoiseCancelling;
  boolean standby;
  boolean forcedMono;

  void setFrequency(float);
  void transmitData();
  void initializeTransmissionData();
  void readStatus();
  float getFrequencyInMHz(unsigned int);
  void calculateOptimalLOInjection(float);
  void setHighSideLOInjection();
  void setLowSideLOInjection();
  void loadFrequency();
  byte isReady();
  byte isBandLimitReached();

public:
  TEA5767N();
  void selectFrequency(float frequency, boolean muteDuringUpdate);
  void selectChannel(int channel, boolean muteDuringUpdate);
  float readFrequencyInMHz();

  void setSearchUp();
  void setSearchDown();
  void setSearchLowStopLevel();
  void setSearchMidStopLevel();
  void setSearchHighStopLevel();

  void toggleMute();
  void toggleMuteLeft();
  void toggleMuteRight();
  void toggleSoftMute();

  void toggleStandby();
  void toggleHighCutControl();
  void toggleStereoNoiseCancelling();
  void toggleForcedMono();

  byte searchNext(boolean muteDuringSearch);
  byte searchFrom(float frequency, boolean muteDuringSearch);
  byte searchFromBeginning(boolean muteDuringSearch);
  byte searchFromEnd(boolean muteDuringSearch);

  byte getSignalLevel();

  byte isStereo();
  byte isSearchUp();
  byte isSearchDown();

  boolean isMuted();
  boolean isMutedLeft();
  boolean isMutedRight();
  boolean isSoftMuted();
  boolean isStandBy();
};

#endif