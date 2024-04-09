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

#include "ApplicationCommand.h"

#include "ApplicationState.h"

ApplicationCommand ApplicationCommand::Dummy()
{
    return {"", "", NONE, 0, {""}, {""}};
}

void ApplicationCommand::clear()
{
    param_ = "";
    command_ = NONE;
    expectedOptions_ = 0;
    optionsDescriptions_ = StringArray({""});
    commandDescriptions_ = StringArray({""});
    opts_.clear();
}

void ApplicationCommand::filter(ApplicationState& state, const MidiMessage& msg, DisplayState& display)
{
    switch (command_)
    {
        case CHANNEL:
            display.channel = state.asDecOrHex7BitValue(opts_[0]);
            break;
        case VOICE:
            display.filtered |= checkChannel(msg, display.channel) &&
                (msg.isNoteOnOrOff() || msg.isAftertouch() || msg.isController() ||
                 msg.isProgramChange() || msg.isChannelPressure() || msg.isPitchWheel());
            break;
        case NOTE:
            display.filtered |= checkChannel(msg, display.channel) &&
                msg.isNoteOnOrOff();
            break;
        case NOTE_ON:
            display.filtered |= checkChannel(msg, display.channel) &&
                msg.isNoteOn() &&
                (opts_.isEmpty() || (msg.getNoteNumber() == state.asNoteNumber(opts_[0])));
            break;
        case NOTE_OFF:
            display.filtered |= checkChannel(msg, display.channel) &&
                msg.isNoteOff() &&
                (opts_.isEmpty() || (msg.getNoteNumber() == state.asNoteNumber(opts_[0])));
            break;
        case POLY_PRESSURE:
            display.filtered |= checkChannel(msg, display.channel) &&
                msg.isAftertouch() &&
                (opts_.isEmpty() || (msg.getNoteNumber() == state.asNoteNumber(opts_[0])));
            break;
        case CONTROL_CHANGE:
            display.displayControlChange = checkChannel(msg, display.channel) &&
                msg.isController() &&
                (opts_.isEmpty() || (msg.getControllerNumber() == state.asDecOrHex7BitValue(opts_[0])));
            display.filtered |= display.displayControlChange;
            break;
        case CONTROL_CHANGE_14BIT:
            if (checkChannel(msg, display.channel) &&
                msg.isController() &&
                msg.getControllerNumber() < 64 &&
                (opts_.isEmpty() || (msg.getControllerNumber() == state.asDecOrHex7BitValue(opts_[0]))))
            {
                uint8 ch = (uint8)msg.getChannel() - 1;
                uint8 cc = (uint8)msg.getControllerNumber();
                uint8 v = (uint8)msg.getControllerValue();
                uint8 prev_v = (uint8)state.lastCC_[ch][cc];
                state.lastCC_[ch][cc] = v;
                
                // handle 14-bit MIDI CC values as appropriate
                if (cc < 32)
                {
                    // only trigger an MSB-initiated change when its value is different than before
                    // if it's the same, wait for the LSB to trigger the change
                    if (v != prev_v)
                    {
                        uint8 lsb_cc = cc + 32;
                        state.lastCC_[ch][lsb_cc] = 0;
                        display.displayControlChange14bit = true;
                    }
                }
                // handle 14-bit MIDI CC LSB values
                else if (cc >= 32 && cc < 64)
                {
                    uint8 msb_cc = cc - 32;
                    int msb = state.lastCC_[ch][msb_cc];
                    if (msb >= 0)
                    {
                        display.displayControlChange14bit = true;
                    }
                }
            }
            
            display.displayControlChange = display.displayControlChange14bit;
            display.filtered |= display.displayControlChange;
            break;
        case NRPN:
        case NRPN_FULL:
            if (checkChannel(msg, display.channel) && msg.isController())
            {
                auto nrpn = state.rpnDetector_.tryParse(msg.getChannel(), msg.getControllerNumber(), msg.getControllerValue());
                if (nrpn.has_value())
                {
                    display.displayNrpn = state.rpnMsg_.isNRPN &&
                        (command_ == NRPN || (command_ == NRPN_FULL && state.rpnMsg_.is14BitValue)) &&
                        (opts_.isEmpty() || (state.rpnMsg_.parameterNumber == state.asDecOrHex14BitValue(opts_[0])));
                        display.filtered |= display.displayNrpn;
                }
            }
            break;
        case RPN:
        case RPN_FULL:
            if (checkChannel(msg, display.channel) && msg.isController())
            {
                auto rpn = state.rpnDetector_.tryParse(msg.getChannel(), msg.getControllerNumber(), msg.getControllerValue());
                if (rpn.has_value())
                {
                    display.displayRpn = !state.rpnMsg_.isNRPN &&
                        (command_ == RPN || (command_ == RPN_FULL && state.rpnMsg_.is14BitValue)) &&
                        (opts_.isEmpty() || (state.rpnMsg_.parameterNumber == state.asDecOrHex14BitValue(opts_[0])));
                        display.filtered |= display.displayRpn;
                }
            }
            break;
        case PROGRAM_CHANGE:
            display.filtered |= checkChannel(msg, display.channel) &&
                msg.isProgramChange() &&
                (opts_.isEmpty() || (msg.getProgramChangeNumber() == state.asDecOrHex7BitValue(opts_[0])));
            break;
        case CHANNEL_PRESSURE:
            display.filtered |= checkChannel(msg, display.channel) &&
                msg.isChannelPressure();
            break;
        case PITCH_BEND:
            display.filtered |= checkChannel(msg, display.channel) &&
                msg.isPitchWheel();
            break;

        case SYSTEM_REALTIME:
            display.filtered |= msg.isMidiClock() || msg.isMidiStart() || msg.isMidiStop() || msg.isMidiContinue() ||
                msg.isActiveSense() || (msg.getRawDataSize() == 1 && msg.getRawData()[0] == 0xff);
            break;
        case CLOCK:
            display.filtered |= msg.isMidiClock();
            break;
        case START:
            display.filtered |= msg.isMidiStart();
            break;
        case STOP:
            display.filtered |= msg.isMidiStop();
            break;
        case CONTINUE:
            display.filtered |= msg.isMidiContinue();
            break;
        case ACTIVE_SENSING:
            display.filtered |= msg.isActiveSense();
            break;
        case RESET:
            display.filtered |= msg.getRawDataSize() == 1 && msg.getRawData()[0] == 0xff;
            break;
            
        case SYSTEM_COMMON:
            display.filtered |= msg.isSysEx() || msg.isQuarterFrame() || msg.isSongPositionPointer() ||
                (msg.getRawDataSize() == 2 && msg.getRawData()[0] == 0xf3) ||
                (msg.getRawDataSize() == 1 && msg.getRawData()[0] == 0xf6);
            break;
        case SYSTEM_EXCLUSIVE:
        case SYSTEM_EXCLUSIVE_FILE:
            display.filtered |= msg.isSysEx();
            break;
        case TIME_CODE:
            display.filtered |= msg.isQuarterFrame();
            break;
        case SONG_POSITION:
            display.filtered |= msg.isSongPositionPointer();
            break;
        case SONG_SELECT:
            display.filtered |= msg.getRawDataSize() == 2 && msg.getRawData()[0] == 0xf3;
            break;
        case TUNE_REQUEST:
            display.filtered |= msg.getRawDataSize() == 1 && msg.getRawData()[0] == 0xf6;
            break;
        default:
            // no-op
            break;
    }
}

bool ApplicationCommand::checkChannel(const MidiMessage& msg, int channel)
{
    return channel == 0 || msg.getChannel() == channel;
}
