/*
    This file is part of the BocchiYM family of cycle-accurate Yamaha FM sound chip emulators.
    Copyright (C) 2023 BueniaDev.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

// BocchiYM3012
// Chip Name: YM3012 (2-channel floating point DAC)
//
// Bocchi's Notes:
// This file contains the implementation of the YM3012 DAC.
// Like with a real YM3012, it is typically meant to be used with its corresponding YM2151 implementation.

#include "bocchi3012.h"
using namespace bocchi3012;
using namespace std;

namespace bocchi3012
{
    Bocchi3012::Bocchi3012()
    {

    }

    Bocchi3012::~Bocchi3012()
    {

    }

    // Initializes the emulated YM3012
    void Bocchi3012::init()
    {
	current_pins = {};
	output.fill(0);
    }

    // Initializes the sample divider variables used for nearest-neighbor resampling
    // (this API function must be called before calling the init function)
    // 
    // Parameters:
    // clock_rate = Desired clock rate of the emulated YM3012, typically 3579545 (3.579545 mHZ)
    // sample_rate = Desired output sample rate (must not be 0)
    void Bocchi3012::setSampleRates(uint32_t clock_rate, uint32_t sample_rate)
    {
	if (sample_rate == 0)
	{
	    cout << "Invalid sample rate detected, choose a different sample rate!" << endl;
	    throw runtime_error("Bocchi3012 error");
	}

	sample_divider = int64_t(float(clock_rate / sample_rate) * (1 << num_frac_bits));
	counter = sample_divider;
    }

    // Ticks the emulated YM3012 forward one-half clock cycle
    //
    // Parameters:
    // clk = Value of clock cycle pulse (either true or false)
    //
    // Sample psuedo code for ticking the emulated YM3012 for 1 clock cycle:
    //
    // tickCLK(true)
    // tickCLK(false)
    void Bocchi3012::tickCLK(bool clk)
    {
	clk_rise = (!prev_clk && clk);
	if (!current_pins.pin_nicl)
	{
	    reg_sr = 0;
	    prev_sh1 = false;
	    prev_sh2 = false;
	    prev_sy = false;
	}
	else if (clk_rise)
	{
	    tickInternal();
	}

	tickValidSample();
	prev_clk = clk;
	prev_res = current_pins.pin_nicl;
    }

    void Bocchi3012::tickInternal()
    {
	if (prev_sy && !current_pins.pin_sy)
	{
	    if (prev_sh1 && !current_pins.pin_sh1)
	    {
		output[1] = calcSample(reg_sr);
	    }

	    if (prev_sh2 && !current_pins.pin_sh2)
	    {
		output[0] = calcSample(reg_sr);
	    }

	    reg_sr = ((reg_sr >> 1) | (current_pins.pin_so << 12));

	    prev_sh1 = current_pins.pin_sh1;
	    prev_sh2 = current_pins.pin_sh2;
	}

	prev_sy = current_pins.pin_sy;
    }

    int16_t Bocchi3012::calcSample(uint16_t latch)
    {
	int exp = ((latch >> 10) & 0x7);
	int mant = ((latch & 0x3FF) - 512);

	if (exp == 0)
	{
	    return 0;
	}

	return ((mant << exp) >> 1);
    }

    void Bocchi3012::tickValidSample()
    {
	valid_sample = false;
	if (clk_rise)
	{
	    counter -= (1 << num_frac_bits);

	    if (counter <= 0)
	    {
		counter += sample_divider;
		final_samples[0] = output[0];
		final_samples[1] = output[1];
		valid_sample = true;
	    }
	}
    }

    // Returns true if a resampled audio sample is available
    bool Bocchi3012::isValidSample()
    {
	return valid_sample;
    }

    // Returns the value of the resampled audio samples (as a single 16-bit stereo sample)
    // Format of returned audio sample:
    // arr[0] = Left channel output
    // arr[1] = Right channel output
    array<int16_t, 2> Bocchi3012::getSamples()
    {
	return final_samples;
    }
};