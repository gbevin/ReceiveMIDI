/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2017 - ROLI Ltd.

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

//==============================================================================
/** This is derived from JUCE's MidiRPNMessage classes and tweaks it for more
    appropriate behavior.
 */
//==============================================================================

#include "MidiRPN.h"

RPNDetector::RPNDetector() noexcept
{
}

RPNDetector::~RPNDetector() noexcept
{
}

bool RPNDetector::parseControllerMessage (int midiChannel,
                                              int controllerNumber,
                                              int controllerValue,
                                              RPNMessage& result) noexcept
{
    jassert (midiChannel >= 1 && midiChannel <= 16);
    jassert (controllerNumber >= 0 && controllerNumber < 128);
    jassert (controllerValue >= 0 && controllerValue < 128);

    return states[midiChannel - 1].handleController (midiChannel, controllerNumber, controllerValue, result);
}

void RPNDetector::reset() noexcept
{
    for (int i = 0; i < 16; ++i)
    {
        states[i].parameterMSB = 0xff;
        states[i].parameterLSB = 0xff;
        states[i].resetValue();
        states[i].isNRPN = false;
    }
}

//==============================================================================
RPNDetector::ChannelState::ChannelState() noexcept
    : parameterMSB (0xff), parameterLSB (0xff), valueMSB (0xff), valueLSB (0xff), isNRPN (false)
{
}

bool RPNDetector::ChannelState::handleController (int channel,
                                                      int controllerNumber,
                                                      int value,
                                                      RPNMessage& result) noexcept
{
    switch (controllerNumber)
    {
        case 0x62:  parameterLSB = uint8 (value); resetValue(); isNRPN = true;  break;
        case 0x63:  parameterMSB = uint8 (value); resetValue(); isNRPN = true;  break;

        case 0x64:  parameterLSB = uint8 (value); resetValue(); isNRPN = false; break;
        case 0x65:  parameterMSB = uint8 (value); resetValue(); isNRPN = false; break;

        case 0x06:  valueMSB = uint8 (value); valueLSB = 0xff; return sendIfReady (channel, result);
        case 0x26:  valueLSB = uint8 (value); return sendIfReady (channel, result);

        default:  break;
    }

    return false;
}

void RPNDetector::ChannelState::resetValue() noexcept
{
    valueMSB = 0xff;
    valueLSB = 0xff;
}

//==============================================================================
bool RPNDetector::ChannelState::sendIfReady (int channel, RPNMessage& result) noexcept
{
    if (parameterMSB < 0x80 && parameterLSB < 0x80)
    {
        if (valueMSB < 0x80)
        {
            result.channel = channel;
            result.parameterNumber = (parameterMSB << 7) + parameterLSB;
            result.isNRPN = isNRPN;

            result.value = (valueMSB << 7);
            result.usesBothMSBandLSB = false;
            
            if (valueLSB < 0x80)
            {
                result.value += valueLSB;
                result.usesBothMSBandLSB = true;
            }

            return true;
        }
    }

    return false;
}
