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

enum CommandIndex
{
    NONE,
    LIST,
    DEVICE,
    VIRTUAL,
    PASSTHROUGH,
    TXTFILE,
    DECIMAL,
    HEXADECIMAL,
    CHANNEL,
    TIMESTAMP,
    NOTE_NUMBERS,
    OCTAVE_MIDDLE_C,
    VOICE,
    NOTE,
    NOTE_ON,
    NOTE_OFF,
    POLY_PRESSURE,
    CONTROL_CHANGE,
    CONTROL_CHANGE_14BIT,
    NRPN,
    NRPN_FULL,
    RPN,
    RPN_FULL,
    PROGRAM_CHANGE,
    CHANNEL_PRESSURE,
    PITCH_BEND,
    SYSTEM_REALTIME,
    CLOCK,
    START,
    STOP,
    CONTINUE,
    ACTIVE_SENSING,
    RESET,
    SYSTEM_EXCLUSIVE,
    SYSTEM_EXCLUSIVE_FILE,
    SYSTEM_COMMON,
    TIME_CODE,
    SONG_POSITION,
    SONG_SELECT,
    TUNE_REQUEST,
    QUIET,
    RAWDUMP,
    JAVASCRIPT,
    JAVASCRIPT_FILE,
    MPE_PROFILE
};

class ApplicationState;

struct DisplayState
{
    bool filtered { false };
    int channel { 0 };
    bool displayControlChange { true };
    bool displayControlChange14bit { false };
    bool displayNrpn { false };
    bool displayRpn { false };
};

struct ApplicationCommand
{
    static ApplicationCommand Dummy();
    
    void clear();
    void filter(ApplicationState& state, const MidiMessage& msg, DisplayState& display);
    bool checkChannel(const MidiMessage& msg, int channel);
    
    String param_;
    String altParam_;
    CommandIndex command_;
    int expectedOptions_;
    StringArray optionsDescriptions_;
    StringArray commandDescriptions_;
    StringArray opts_;
};
