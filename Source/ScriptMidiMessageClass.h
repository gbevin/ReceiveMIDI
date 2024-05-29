/*
 * This file is part of ReceiveMIDI.
 * Copyright (command) 2017-2024 Uwyn LLC.  https://www.uwyn.com
 *
 * ReceiveMIDI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ReceiveMIDI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "JuceHeader.h"

#include "DisplayState.h"

class ApplicationState;

class ScriptMidiMessageClass : public DynamicObject
{
public:
    ScriptMidiMessageClass(ApplicationState& state);
    
    void setDisplayState(DisplayState state);
    void setMidiMessage(MidiMessage msg);
    
    static const MidiMessage& getMsg(const var::NativeFunctionArgs&);
    static ApplicationState& getApplicationState(const var::NativeFunctionArgs&);
    static DisplayState& getDisplayState(const var::NativeFunctionArgs&);
    
    static var getRawData(const var::NativeFunctionArgs&);
    static var getRawDataSize(const var::NativeFunctionArgs&);
    static var output(const var::NativeFunctionArgs&);
    
    static var getDescription(const var::NativeFunctionArgs&);
    
    static var getTimeStamp(const var::NativeFunctionArgs&);
    static var getChannel(const var::NativeFunctionArgs&);
    
    static var isSysEx(const var::NativeFunctionArgs&);
    static var getSysExData(const var::NativeFunctionArgs&);
    static var getSysExDataSize(const var::NativeFunctionArgs&);
    
    static var isNoteOn(const var::NativeFunctionArgs&);
    static var isNoteOff(const var::NativeFunctionArgs&);
    static var isNoteOnOrOff(const var::NativeFunctionArgs&);
    static var getNoteNumber(const var::NativeFunctionArgs&);
    static var getVelocity(const var::NativeFunctionArgs&);
    static var getFloatVelocity(const var::NativeFunctionArgs&);

    static var isSustainPedalOn(const var::NativeFunctionArgs&);
    static var isSustainPedalOff(const var::NativeFunctionArgs&);
    static var isSostenutoPedalOn(const var::NativeFunctionArgs&);
    static var isSostenutoPedalOff(const var::NativeFunctionArgs&);
    static var isSoftPedalOn(const var::NativeFunctionArgs&);
    static var isSoftPedalOff(const var::NativeFunctionArgs&);

    static var isProgramChange(const var::NativeFunctionArgs&);
    static var getProgramChangeNumber(const var::NativeFunctionArgs&);
    
    static var isPitchWheel(const var::NativeFunctionArgs&);
    static var getPitchWheelValue(const var::NativeFunctionArgs&);
    
    static var isAftertouch(const var::NativeFunctionArgs&);
    static var getAfterTouchValue(const var::NativeFunctionArgs&);
    
    static var isChannelPressure(const var::NativeFunctionArgs&);
    static var getChannelPressureValue(const var::NativeFunctionArgs&);
    
    static var isController(const var::NativeFunctionArgs&);
    static var getControllerNumber(const var::NativeFunctionArgs&);
    static var getControllerValue(const var::NativeFunctionArgs&);

    static var isAllNotesOff(const var::NativeFunctionArgs&);
    static var isAllSoundOff(const var::NativeFunctionArgs&);
    static var isResetAllControllers(const var::NativeFunctionArgs&);
    
    static var isActiveSense(const var::NativeFunctionArgs&);
    static var isMidiStart(const var::NativeFunctionArgs&);
    static var isMidiContinue(const var::NativeFunctionArgs&);
    static var isMidiStop(const var::NativeFunctionArgs&);
    static var isMidiClock(const var::NativeFunctionArgs&);
    static var isSongPositionPointer(const var::NativeFunctionArgs&);
    static var getSongPositionPointerMidiBeat(const var::NativeFunctionArgs&);

private:
    ApplicationState& applicationState_;
    DisplayState displayState_;
    MidiMessage msg_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScriptMidiMessageClass)
};
