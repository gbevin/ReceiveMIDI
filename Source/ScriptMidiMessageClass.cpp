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

#include "ScriptMidiMessageClass.h"

#include "ApplicationState.h"

ScriptMidiMessageClass::ScriptMidiMessageClass(ApplicationState& state) : applicationState_(state)
{
    setMethod("getRawData", getRawData);
    setMethod("rawData", getRawData);
    setMethod("getRawDataSize", getRawDataSize);
    setMethod("rawDataSize", getRawDataSize);
    setMethod("output", output);

    setMethod("getDescription", getDescription);
    setMethod("description", getDescription);

    setMethod("getTimeStamp", getTimeStamp);
    setMethod("timeStamp", getTimeStamp);
    setMethod("getChannel", getChannel);
    setMethod("channel", getChannel);
    
    setMethod("isSysEx", isSysEx);
    setMethod("getSysExData", getSysExData);
    setMethod("sysExData", getSysExData);
    setMethod("getSysExDataSize", getSysExDataSize);
    setMethod("sysExDataSize", getSysExDataSize);
    
    setMethod("isNoteOn", isNoteOn);
    setMethod("isNoteOff", isNoteOff);
    setMethod("isNoteOnOrOff", isNoteOnOrOff);
    setMethod("getNoteNumber", getNoteNumber);
    setMethod("noteNumber", getNoteNumber);
    setMethod("getVelocity", getVelocity);
    setMethod("velocity", getVelocity);
    setMethod("getFloatVelocity", getFloatVelocity);
    setMethod("floatVelocity", getFloatVelocity);

    setMethod("isSustainPedalOn", isSustainPedalOn);
    setMethod("isSustainPedalOff", isSustainPedalOff);
    setMethod("isSostenutoPedalOn", isSostenutoPedalOn);
    setMethod("isSostenutoPedalOff", isSostenutoPedalOff);
    setMethod("isSoftPedalOn", isSoftPedalOn);
    setMethod("isSoftPedalOff", isSoftPedalOff);

    setMethod("isProgramChange", isProgramChange);
    setMethod("getProgramChangeNumber", getProgramChangeNumber);
    setMethod("programChange", getProgramChangeNumber);
    
    setMethod("isPitchWheel", isPitchWheel);
    setMethod("isPitchBend", isPitchWheel);
    setMethod("getPitchWheelValue", getPitchWheelValue);
    setMethod("getPitchBendValue", getPitchWheelValue);
    setMethod("pitchWheel", getPitchWheelValue);
    setMethod("pitchBend", getPitchWheelValue);
    
    setMethod("isAftertouch", isAftertouch);
    setMethod("isPolyPressure", isAftertouch);
    setMethod("getAfterTouchValue", getAfterTouchValue);
    setMethod("getPolyPressureValue", getAfterTouchValue);
    setMethod("afterTouch", getAfterTouchValue);
    setMethod("polyPressure", getAfterTouchValue);
    
    setMethod("isChannelPressure", isChannelPressure);
    setMethod("getChannelPressureValue", getChannelPressureValue);
    setMethod("channelPressure", getChannelPressureValue);
    
    setMethod("isController", isController);
    setMethod("getControllerNumber", getControllerNumber);
    setMethod("controllerNumber", getControllerNumber);
    setMethod("getControllerValue", getControllerValue);
    setMethod("controllerValue", getControllerValue);
    
    setMethod("isAllNotesOff", isAllNotesOff);
    setMethod("isAllSoundOff", isAllSoundOff);
    setMethod("isResetAllControllers", isResetAllControllers);
    
    setMethod("isActiveSense", isActiveSense);
    setMethod("isMidiStart", isMidiStart);
    setMethod("isMidiContinue", isMidiContinue);
    setMethod("isMidiStop", isMidiStop);
    setMethod("isMidiClock", isMidiClock);
    setMethod("isSongPositionPointer", isSongPositionPointer);
    setMethod("getSongPositionPointerMidiBeat", getSongPositionPointerMidiBeat);
    setMethod("songPositionPointerMidiBeat", getSongPositionPointerMidiBeat);
}

void ScriptMidiMessageClass::setDisplayState(DisplayState state)                                { displayState_ = state; }
void ScriptMidiMessageClass::setMidiMessage(MidiMessage msg)                                    { msg_ = msg; }

const MidiMessage& ScriptMidiMessageClass::getMsg(const var::NativeFunctionArgs& a)             { return ((ScriptMidiMessageClass*)a.thisObject.getDynamicObject())->msg_; }
ApplicationState& ScriptMidiMessageClass::getApplicationState(const var::NativeFunctionArgs& a) { return ((ScriptMidiMessageClass*)a.thisObject.getDynamicObject())->applicationState_; }
DisplayState& ScriptMidiMessageClass::getDisplayState(const var::NativeFunctionArgs& a)         { return ((ScriptMidiMessageClass*)a.thisObject.getDynamicObject())->displayState_; }

var ScriptMidiMessageClass::getRawDataSize(const var::NativeFunctionArgs& a)                    { return getMsg(a).getRawDataSize(); }
var ScriptMidiMessageClass::getDescription(const var::NativeFunctionArgs& a)                    { return getMsg(a).getDescription(); }
var ScriptMidiMessageClass::getTimeStamp(const var::NativeFunctionArgs& a)                      { return getMsg(a).getTimeStamp(); }
var ScriptMidiMessageClass::getChannel(const var::NativeFunctionArgs& a)                        { return getMsg(a).getChannel(); }

var ScriptMidiMessageClass::isSysEx(const var::NativeFunctionArgs& a)                           { return getMsg(a).isSysEx(); }
var ScriptMidiMessageClass::getSysExDataSize(const var::NativeFunctionArgs& a)                  { return getMsg(a).getSysExDataSize(); }

var ScriptMidiMessageClass::isNoteOn(const var::NativeFunctionArgs& a)                          { return getMsg(a).isNoteOn(); }
var ScriptMidiMessageClass::isNoteOff(const var::NativeFunctionArgs& a)                         { return getMsg(a).isNoteOff(); }
var ScriptMidiMessageClass::isNoteOnOrOff(const var::NativeFunctionArgs& a)                     { return getMsg(a).isNoteOnOrOff(); }
var ScriptMidiMessageClass::getNoteNumber(const var::NativeFunctionArgs& a)                     { return getMsg(a).getNoteNumber(); }
var ScriptMidiMessageClass::getVelocity(const var::NativeFunctionArgs& a)                       { return getMsg(a).getVelocity(); }
var ScriptMidiMessageClass::getFloatVelocity(const var::NativeFunctionArgs& a)                  { return getMsg(a).getFloatVelocity(); }

var ScriptMidiMessageClass::isSustainPedalOn(const var::NativeFunctionArgs& a)                  { return getMsg(a).isSustainPedalOn(); }
var ScriptMidiMessageClass::isSustainPedalOff(const var::NativeFunctionArgs& a)                 { return getMsg(a).isSustainPedalOff(); }
var ScriptMidiMessageClass::isSostenutoPedalOn(const var::NativeFunctionArgs& a)                { return getMsg(a).isSostenutoPedalOn(); }
var ScriptMidiMessageClass::isSostenutoPedalOff(const var::NativeFunctionArgs& a)               { return getMsg(a).isSostenutoPedalOff(); }
var ScriptMidiMessageClass::isSoftPedalOn(const var::NativeFunctionArgs& a)                     { return getMsg(a).isSoftPedalOn(); }
var ScriptMidiMessageClass::isSoftPedalOff(const var::NativeFunctionArgs& a)                    { return getMsg(a).isSoftPedalOff(); }

var ScriptMidiMessageClass::isProgramChange(const var::NativeFunctionArgs& a)                   { return getMsg(a).isProgramChange(); }
var ScriptMidiMessageClass::getProgramChangeNumber(const var::NativeFunctionArgs& a)            { return getMsg(a).getProgramChangeNumber(); }

var ScriptMidiMessageClass::isPitchWheel(const var::NativeFunctionArgs& a)                      { return getMsg(a).isPitchWheel(); }
var ScriptMidiMessageClass::getPitchWheelValue(const var::NativeFunctionArgs& a)                { return getMsg(a).getPitchWheelValue(); }

var ScriptMidiMessageClass::isAftertouch(const var::NativeFunctionArgs& a)                      { return getMsg(a).isAftertouch(); }
var ScriptMidiMessageClass::getAfterTouchValue(const var::NativeFunctionArgs& a)                { return getMsg(a).getAfterTouchValue(); }

var ScriptMidiMessageClass::isChannelPressure(const var::NativeFunctionArgs& a)                 { return getMsg(a).isChannelPressure(); }
var ScriptMidiMessageClass::getChannelPressureValue(const var::NativeFunctionArgs& a)           { return getMsg(a).getChannelPressureValue(); }

var ScriptMidiMessageClass::isController(const var::NativeFunctionArgs& a)                      { return getMsg(a).isController(); }
var ScriptMidiMessageClass::getControllerNumber(const var::NativeFunctionArgs& a)               { return getMsg(a).getControllerNumber(); }
var ScriptMidiMessageClass::getControllerValue(const var::NativeFunctionArgs& a)                { return getMsg(a).getControllerValue(); }

var ScriptMidiMessageClass::isAllNotesOff(const var::NativeFunctionArgs& a)                     { return getMsg(a).isAllNotesOff(); }
var ScriptMidiMessageClass::isAllSoundOff(const var::NativeFunctionArgs& a)                     { return getMsg(a).isAllSoundOff(); }
var ScriptMidiMessageClass::isResetAllControllers(const var::NativeFunctionArgs& a)             { return getMsg(a).isResetAllControllers(); }

var ScriptMidiMessageClass::isActiveSense(const var::NativeFunctionArgs& a)                     { return getMsg(a).isActiveSense(); }
var ScriptMidiMessageClass::isMidiStart(const var::NativeFunctionArgs& a)                       { return getMsg(a).isMidiStart(); }
var ScriptMidiMessageClass::isMidiContinue(const var::NativeFunctionArgs& a)                    { return getMsg(a).isMidiContinue(); }
var ScriptMidiMessageClass::isMidiStop(const var::NativeFunctionArgs& a)                        { return getMsg(a).isMidiStop(); }
var ScriptMidiMessageClass::isMidiClock(const var::NativeFunctionArgs& a)                       { return getMsg(a).isMidiClock(); }
var ScriptMidiMessageClass::isSongPositionPointer(const var::NativeFunctionArgs& a)             { return getMsg(a).isSongPositionPointer(); }
var ScriptMidiMessageClass::getSongPositionPointerMidiBeat(const var::NativeFunctionArgs& a)    { return getMsg(a).getSongPositionPointerMidiBeat(); }

var ScriptMidiMessageClass::getRawData(const var::NativeFunctionArgs& a)
{
    Array<var> data;
    
    const uint8* raw = getMsg(a).getRawData();
    int size = getMsg(a).getRawDataSize();
    for (int i = 0; i < size; ++i)
    {
        data.add(raw[i]);
    }
    
    return data;
}

var ScriptMidiMessageClass::getSysExData(const var::NativeFunctionArgs& a)
{
    Array<var> data;
    
    const uint8* raw = getMsg(a).getSysExData();
    int size = getMsg(a).getSysExDataSize();
    for (int i = 0; i < size; ++i)
    {
        data.add(raw[i]);
    }
    
    return data;
}

var ScriptMidiMessageClass::output(const var::NativeFunctionArgs& a)
{
    getApplicationState(a).outputMessage(getMsg(a), getDisplayState(a));
    
    return true;
}
