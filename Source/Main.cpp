/*
 * This file is part of ReceiveMIDI.
 * Copyright (command) 2017-2019 Uwyn SPRL.  http://www.uwyn.com
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
#include "JuceHeader.h"

#include "MidiRPN.h"
#include "ScriptMidiMessageClass.h"
#include "ScriptOscClass.h"
#include "ScriptUtilClass.h"

#include <sstream>

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
    JAVASCRIPT_FILE
};

static const int DEFAULT_OCTAVE_MIDDLE_C = 3;
static const String& DEFAULT_VIRTUAL_NAME = "ReceiveMIDI";

struct ApplicationCommand
{
    static ApplicationCommand Dummy()
    {
        return {"", "", NONE, 0, "", ""};
    }
    
    void clear()
    {
        param_ = "";
        command_ = NONE;
        expectedOptions_ = 0;
        optionsDescription_ = "";
        commandDescription_ = "";
        opts_.clear();
    }
    
    String param_;
    String altParam_;
    CommandIndex command_;
    int expectedOptions_;
    String optionsDescription_;
    String commandDescription_;
    StringArray opts_;
};

inline float sign(float value)
{
    return (float)(value > 0.) - (value < 0.);
}

class receiveMidiApplication  : public JUCEApplicationBase, public MidiInputCallback, public Timer
{
public:
    receiveMidiApplication()
    {
        commands_.add({"dev",   "device",                   DEVICE,                1, "name",           "Set the name of the MIDI input port"});
        commands_.add({"virt",  "virtual",                  VIRTUAL,              -1, "(name)",         "Use virtual MIDI port with optional name (Linux/macOS)"});
        commands_.add({"pass",  "pass-through",             PASSTHROUGH,           1, "name",           "Set name of MIDI output port for MIDI pass-through"});
        commands_.add({"list",  "",                         LIST,                  0, "",               "Lists the MIDI input ports"});
        commands_.add({"file",  "",                         TXTFILE,               1, "path",           "Loads commands from the specified program file"});
        commands_.add({"dec",   "decimal",                  DECIMAL,               0, "",               "Interpret the next numbers as decimals by default"});
        commands_.add({"hex",   "hexadecimal",              HEXADECIMAL,           0, "",               "Interpret the next numbers as hexadecimals by default"});
        commands_.add({"ch",    "channel",                  CHANNEL,               1, "number",         "Set MIDI channel for the commands (0-16), defaults to 0"});
        commands_.add({"ts",    "timestamp",                TIMESTAMP,             0, "",               "Output a timestamp for each received MIDI message"});
        commands_.add({"nn",    "note-numbers",             NOTE_NUMBERS,          0, "",               "Output notes as numbers instead of names"});
        commands_.add({"omc",   "octave-middle-c",          OCTAVE_MIDDLE_C,       1, "number",         "Set octave for middle C, defaults to 3"});
        commands_.add({"voice", "",                         VOICE,                 0, "",               "Show all Channel Voice messages"});
        commands_.add({"note",  "",                         NOTE,                  0, "",               "Show all Note messages"});
        commands_.add({"on",    "note-on",                  NOTE_ON,              -1, "(note)",         "Show Note On, optionally for note (0-127)"});
        commands_.add({"off",   "note-off",                 NOTE_OFF,             -1, "(note)",         "Show Note Off, optionally for note (0-127)"});
        commands_.add({"pp",    "poly-pressure",            POLY_PRESSURE,        -1, "(note)",         "Show Poly Pressure, optionally for note (0-127)"});
        commands_.add({"cc",    "control-change",           CONTROL_CHANGE,       -1, "(number)",       "Show Control Change, optionally for controller (0-127)"});
        commands_.add({"cc14",  "control-change-14",        CONTROL_CHANGE_14BIT, -1, "(number)",       "Show 14-bit CC, optionally for controller (0-63)"});
        commands_.add({"nrpn",  "",                         NRPN,                 -1, "(number)",       "Show NRPN, optionally for parameter (0-16383)"});
        commands_.add({"nrpnf", "nrpn-full",                NRPN_FULL,            -1, "(number)",       "Show full NRPN (MSB+LSB), optionally for parameter (0-16383)"});
        commands_.add({"rpn",   "",                         RPN,                  -1, "(number)",       "Show RPN, optionally for parameter (0-16383)"});
        commands_.add({"rpnf",  "rpn-full",                 RPN_FULL,             -1, "(number)",       "Show full RPN (MSB+LSB), optionally for parameter (0-16383)"});
        commands_.add({"pc",    "program-change",           PROGRAM_CHANGE,       -1, "(number)",       "Show Program Change, optionally for program (0-127)"});
        commands_.add({"cp",    "channel-pressure",         CHANNEL_PRESSURE,      0, "",               "Show Channel Pressure"});
        commands_.add({"pb",    "pitch-bend",               PITCH_BEND,            0, "",               "Show Pitch Bend"});
        commands_.add({"sr",    "system-realtime",          SYSTEM_REALTIME,       0, "",               "Show all System Real-Time messages"});
        commands_.add({"clock", "",                         CLOCK,                 0, "",               "Show Timing Clock"});
        commands_.add({"start", "",                         START,                 0, "",               "Show Start"});
        commands_.add({"stop",  "",                         STOP,                  0, "",               "Show Stop"});
        commands_.add({"cont",  "continue",                 CONTINUE,              0, "",               "Show Continue"});
        commands_.add({"as",    "active-sensing",           ACTIVE_SENSING,        0, "",               "Show Active Sensing"});
        commands_.add({"rst",   "reset",                    RESET,                 0, "",               "Show Reset"});
        commands_.add({"sc",    "system-common",            SYSTEM_COMMON,         0, "",               "Show all System Common messages"});
        commands_.add({"syx",   "system-exclusive",         SYSTEM_EXCLUSIVE,      0, "",               "Show System Exclusive"});
        commands_.add({"syf",   "system-exclusive-file",    SYSTEM_EXCLUSIVE_FILE, 1, "path",           "Store SysEx into a .syx file"});
        commands_.add({"tc",    "time-code",                TIME_CODE,             0, "",               "Show MIDI Time Code Quarter Frame"});
        commands_.add({"spp",   "song-position",            SONG_POSITION,         0, "",               "Show Song Position Pointer"});
        commands_.add({"ss",    "song-select",              SONG_SELECT,           0, "",               "Show Song Select"});
        commands_.add({"tun",   "tune-request",             TUNE_REQUEST,          0, "",               "Show Tune Request"});
        commands_.add({"q",     "quiet",                    QUIET,                 0, "",               "Don't show the received messages on standard output"});
        commands_.add({"dump",  "",                         RAWDUMP,               0, "",               "Dump the received messages 1:1 on standard output"});
        commands_.add({"js",    "javascript",               JAVASCRIPT,            1, "code",           "Execute this script for each received MIDI message"});
        commands_.add({"jsf",   "javascript-file",          JAVASCRIPT_FILE,       1, "path",           "Execute the script in this file for each message"});

        timestampOutput_ = false;
        noteNumbersOutput_ = false;
        octaveMiddleC_ = DEFAULT_OCTAVE_MIDDLE_C;
        useHexadecimalsByDefault_ = false;
        quiet_ = false;
        rawdump_ = false;
        currentCommand_ = ApplicationCommand::Dummy();
        // initialize last CC MSB values
        for (int ch = 0; ch < 16; ++ch)
        {
            for (int cc = 0; cc < 128; ++cc)
            {
                lastCC_[ch][cc] = -1;
            }
        }
    }
    
    const String getApplicationName() override       { return ProjectInfo::projectName; }
    const String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override       { return true; }
    void systemRequestedQuit() override              { quit(); }
    
    void initialise(const String&) override
    {
        StringArray cmdLineParams(getCommandLineParameterArray());
        if (cmdLineParams.contains("--help") || cmdLineParams.contains("-h"))
        {
            printUsage();
            systemRequestedQuit();
            return;
        }
        else if (cmdLineParams.contains("--version"))
        {
            printVersion();
            systemRequestedQuit();
            return;
        }
        
        scriptEngine_.maximumExecutionTime = RelativeTime::days(365);
        scriptMidiMessage_ = new ScriptMidiMessageClass();
        scriptEngine_.registerNativeObject("MIDI",  scriptMidiMessage_);
        scriptEngine_.registerNativeObject("OSC",   new ScriptOscClass());
        scriptEngine_.registerNativeObject("Util",  new ScriptUtilClass());
        
        parseParameters(cmdLineParams);
        
        if (cmdLineParams.contains("--"))
        {
            while (std::cin)
            {
                std::string line;
                getline(std::cin, line);
                StringArray params = parseLineAsParameters(line);
                parseParameters(params);
            }
        }
        
        if (cmdLineParams.isEmpty())
        {
            printUsage();
            systemRequestedQuit();
        }
        else
        {
            startTimer(200);
        }
    }
    
    bool isMidiInDeviceAvailable(const String& name)
    {
        auto devices = MidiInput::getAvailableDevices();
        for (int i = 0; i < devices.size(); ++i)
        {
            if (devices[i].name == name)
            {
                return true;
            }
        }
        return false;
    }
    
    void timerCallback() override
    {
        if (fullMidiInName_.isNotEmpty() && !isMidiInDeviceAvailable(fullMidiInName_))
        {
            std::cerr << "MIDI input port \"" << fullMidiInName_ << "\" got disconnected, waiting." << std::endl;
            
            fullMidiInName_ = String();
            midiIn_ = nullptr;
        }
        else if ((midiInName_.isNotEmpty() && midiIn_ == nullptr))
        {
            if (tryToConnectMidiInput())
            {
                std::cerr << "Connected to MIDI input port \"" << fullMidiInName_ << "\"." << std::endl;
            }
        }
    }
    
    void shutdown() override
    {
        if (sysexOutput_)
        {
            sysexOutput_->flush();
            sysexOutput_.reset();
        }
    }
    void suspended() override {}
    void resumed() override {}
    void anotherInstanceStarted(const String&) override {}
    void unhandledException(const std::exception*, const String&, int) override { jassertfalse; }
    
private:
    ApplicationCommand* findApplicationCommand(const String& param)
    {
        for (auto&& cmd : commands_)
        {
            if (cmd.param_.equalsIgnoreCase(param) || cmd.altParam_.equalsIgnoreCase(param))
            {
                return &cmd;
            }
        }
        return nullptr;
    }
    
    StringArray parseLineAsParameters(const String& line)
    {
        StringArray parameters;
        if (!line.startsWith("#"))
        {
            StringArray tokens;
            tokens.addTokens(line, true);
            tokens.removeEmptyStrings(true);
            for (String token : tokens)
            {
                parameters.add(token.trimCharactersAtStart("\"").trimCharactersAtEnd("\""));
            }
        }
        return parameters;
    }
    
    void executeCurrentCommand()
    {
        ApplicationCommand cmd = currentCommand_;
        currentCommand_ = ApplicationCommand::Dummy();
        executeCommand(cmd);
    }

    void handleVarArgCommand()
    {
        if (currentCommand_.expectedOptions_ < 0)
        {
            executeCurrentCommand();
        }
    }
    
    void parseParameters(StringArray& parameters)
    {
        for (String param : parameters)
        {
            if (param == "--") continue;
            
            ApplicationCommand* cmd = findApplicationCommand(param);
            if (cmd)
            {
                // handle configuration commands immediately without setting up a new one
                switch (cmd->command_)
                {
                    case DECIMAL:
                        useHexadecimalsByDefault_ = false;
                        break;
                    case HEXADECIMAL:
                        useHexadecimalsByDefault_ = true;
                        break;
                    default:
                        handleVarArgCommand();
                        
                        currentCommand_ = *cmd;
                        break;
                }
            }
            else if (currentCommand_.command_ == NONE)
            {
                File file = File::getCurrentWorkingDirectory().getChildFile(param);
                if (file.existsAsFile())
                {
                    parseFile(file);
                }
            }
            else if (currentCommand_.expectedOptions_ != 0)
            {
                currentCommand_.opts_.add(param);
                currentCommand_.expectedOptions_ -= 1;
            }
            
            // handle fixed arg commands
            if (currentCommand_.expectedOptions_ == 0)
            {
                executeCurrentCommand();
            }
        }
        
        handleVarArgCommand();
    }
    
    void parseFile(File file)
    {
        StringArray parameters;
        
        StringArray lines;
        file.readLines(lines);
        for (String line : lines)
        {
            parameters.addArray(parseLineAsParameters(line));
        }
        
        parseParameters(parameters);
    }
    
    bool checkChannel(const MidiMessage& msg, int channel)
    {
        return channel == 0 || msg.getChannel() == channel;
    }
    
    void handleIncomingMidiMessage(MidiInput*, const MidiMessage& msg) override
    {
        bool display_control_change = true;
        bool display_control_change_14bit = false;
        bool display_nrpn = false;
        bool display_rpn = false;

        if (!filterCommands_.isEmpty())
        {
            // handle individual commands
            bool filtered = false;
            int channel = 0;
            display_control_change = false;
            display_control_change_14bit = false;
            
            for (ApplicationCommand& cmd : filterCommands_)
            {
                switch (cmd.command_)
                {
                    case CHANNEL:
                        channel = asDecOrHex7BitValue(cmd.opts_[0]);
                        break;
                    case VOICE:
                        filtered |= checkChannel(msg, channel) &&
                                    (msg.isNoteOnOrOff() || msg.isAftertouch() || msg.isController() ||
                                     msg.isProgramChange() || msg.isChannelPressure() || msg.isPitchWheel());
                        break;
                    case NOTE:
                        filtered |= checkChannel(msg, channel) &&
                                    msg.isNoteOnOrOff();
                        break;
                    case NOTE_ON:
                        filtered |= checkChannel(msg, channel) &&
                                    msg.isNoteOn() &&
                                    (cmd.opts_.isEmpty() || (msg.getNoteNumber() == asNoteNumber(cmd.opts_[0])));
                        break;
                    case NOTE_OFF:
                        filtered |= checkChannel(msg, channel) &&
                                    msg.isNoteOff() &&
                                    (cmd.opts_.isEmpty() || (msg.getNoteNumber() == asNoteNumber(cmd.opts_[0])));
                        break;
                    case POLY_PRESSURE:
                        filtered |= checkChannel(msg, channel) &&
                                    msg.isAftertouch() &&
                                    (cmd.opts_.isEmpty() || (msg.getNoteNumber() == asNoteNumber(cmd.opts_[0])));
                        break;
                    case CONTROL_CHANGE:
                        display_control_change = checkChannel(msg, channel) &&
                                    msg.isController() &&
                                    (cmd.opts_.isEmpty() || (msg.getControllerNumber() == asDecOrHex7BitValue(cmd.opts_[0])));
                        filtered |= display_control_change;
                        break;
                    case CONTROL_CHANGE_14BIT:
                        if (checkChannel(msg, channel) &&
                            msg.isController() &&
                            msg.getControllerNumber() < 64 &&
                            (cmd.opts_.isEmpty() || (msg.getControllerNumber() == asDecOrHex7BitValue(cmd.opts_[0]))))
                        {
                            uint8 ch = msg.getChannel() - 1;
                            uint8 cc = msg.getControllerNumber();
                            uint8 v = msg.getControllerValue();
                            uint8 prev_v = lastCC_[ch][cc];
                            lastCC_[ch][cc] = v;

                            // handle 14-bit MIDI CC values as appropriate
                            if (cc < 32)
                            {
                                // only trigger an MSB-initiated change when its value is different than before
                                // if it's the same, wait for the LSB to trigger the change
                                if (v != prev_v)
                                {
                                    uint8 lsb_cc = cc + 32;
                                    lastCC_[ch][lsb_cc] = 0;
                                    display_control_change_14bit = true;
                                }
                            }
                            // handle 14-bit MIDI CC LSB values
                            else if (cc >= 32 && cc < 64)
                            {
                                uint8 msb_cc = cc - 32;
                                char msb = lastCC_[ch][msb_cc];
                                if (msb >= 0)
                                {
                                    display_control_change_14bit = true;
                                }
                            }
                        }

                        display_control_change = display_control_change_14bit;
                        filtered |= display_control_change;
                        break;
                    case NRPN:
                    case NRPN_FULL:
                        if (checkChannel(msg, channel) && msg.isController())
                        {
                            if (rpnDetector_.parseControllerMessage(msg.getChannel(), msg.getControllerNumber(), msg.getControllerValue(), rpnMsg_))
                            {
                                display_nrpn = rpnMsg_.isNRPN &&
                                    (cmd.command_ == NRPN || (cmd.command_ == NRPN_FULL && rpnMsg_.usesBothMSBandLSB)) &&
                                    (cmd.opts_.isEmpty() || (rpnMsg_.parameterNumber == asDecOrHex14BitValue(cmd.opts_[0])));
                                filtered |= display_nrpn;
                            }
                        }
                        break;
                    case RPN:
                    case RPN_FULL:
                        if (checkChannel(msg, channel) && msg.isController())
                        {
                            if (rpnDetector_.parseControllerMessage(msg.getChannel(), msg.getControllerNumber(), msg.getControllerValue(), rpnMsg_))
                            {
                                display_rpn = !rpnMsg_.isNRPN &&
                                    (cmd.command_ == RPN || (cmd.command_ == RPN_FULL && rpnMsg_.usesBothMSBandLSB)) &&
                                    (cmd.opts_.isEmpty() || (rpnMsg_.parameterNumber == asDecOrHex14BitValue(cmd.opts_[0])));
                                filtered |= display_rpn;
                            }
                        }
                        break;
                    case PROGRAM_CHANGE:
                        filtered |= checkChannel(msg, channel) &&
                                    msg.isProgramChange() &&
                                    (cmd.opts_.isEmpty() || (msg.getProgramChangeNumber() == asDecOrHex7BitValue(cmd.opts_[0])));
                        break;
                    case CHANNEL_PRESSURE:
                        filtered |= checkChannel(msg, channel) &&
                                    msg.isChannelPressure();
                        break;
                    case PITCH_BEND:
                        filtered |= checkChannel(msg, channel) &&
                                    msg.isPitchWheel();
                        break;
                        
                    case SYSTEM_REALTIME:
                        filtered |= msg.isMidiClock() || msg.isMidiStart() || msg.isMidiStop() || msg.isMidiContinue() ||
                                    msg.isActiveSense() || (msg.getRawDataSize() == 1 && msg.getRawData()[0] == 0xff);
                        break;
                    case CLOCK:
                        filtered |= msg.isMidiClock();
                        break;
                    case START:
                        filtered |= msg.isMidiStart();
                        break;
                    case STOP:
                        filtered |= msg.isMidiStop();
                        break;
                    case CONTINUE:
                        filtered |= msg.isMidiContinue();
                        break;
                    case ACTIVE_SENSING:
                        filtered |= msg.isActiveSense();
                        break;
                    case RESET:
                        filtered |= msg.getRawDataSize() == 1 && msg.getRawData()[0] == 0xff;
                        break;
                        
                    case SYSTEM_COMMON:
                        filtered |= msg.isSysEx() || msg.isQuarterFrame() || msg.isSongPositionPointer() ||
                                    (msg.getRawDataSize() == 2 && msg.getRawData()[0] == 0xf3) ||
                                    (msg.getRawDataSize() == 1 && msg.getRawData()[0] == 0xf6);
                        break;
                    case SYSTEM_EXCLUSIVE:
                    case SYSTEM_EXCLUSIVE_FILE:
                        filtered |= msg.isSysEx();
                        break;
                    case TIME_CODE:
                        filtered |= msg.isQuarterFrame();
                        break;
                    case SONG_POSITION:
                        filtered |= msg.isSongPositionPointer();
                        break;
                    case SONG_SELECT:
                        filtered |= msg.getRawDataSize() == 2 && msg.getRawData()[0] == 0xf3;
                        break;
                    case TUNE_REQUEST:
                        filtered |= msg.getRawDataSize() == 1 && msg.getRawData()[0] == 0xf6;
                        break;
                    default:
                        // no-op
                        break;
                }
            }
            
            if (!filtered)
            {
                return;
            }
        }
        
        if (midiOut_)
        {
            midiOut_->sendMessageNow(msg);
        }

        if (scriptCode_.isNotEmpty())
        {
            scriptMidiMessage_->setMidiMessage(msg);
            scriptEngine_.execute(scriptCode_);
        }
        
        if (!quiet_)
        {
            if (rawdump_) {
                dumpMessage(msg);
            }
            else {
                outputMessage(msg, display_control_change, display_control_change_14bit, display_nrpn, display_rpn);
            }
        }
    }
    

    void dumpMessage(const MidiMessage& msg)
    {
        std::cout.write((char *)msg.getRawData(), msg.getRawDataSize());
        std::cout.flush();
    }


    void outputMessage(const MidiMessage& msg, bool displayControlChange, bool displayControlChange14Bit, bool displayNrpn, bool displayRpn)
    {
        if (timestampOutput_)
        {
            Time t = Time::getCurrentTime();
            std::cout << String(t.getHours()).paddedLeft('0', 2) << ":"
                      << String(t.getMinutes()).paddedLeft('0', 2) << ":"
                      << String(t.getSeconds()).paddedLeft('0', 2) << "."
                      << String(t.getMilliseconds()).paddedLeft('0', 3) << "   ";
        }
        
        if (msg.isNoteOn())
        {
            std::cout << "channel "  << outputChannel(msg) << "   " <<
                         "note-on         " << outputNote(msg) << " " << output7Bit(msg.getVelocity()).paddedLeft(' ', 3) << std::endl;
        }
        else if (msg.isNoteOff())
        {
            std::cout << "channel "  << outputChannel(msg) << "   " <<
                         "note-off        " << outputNote(msg) << " " << output7Bit(msg.getVelocity()).paddedLeft(' ', 3) << std::endl;
        }
        else if (msg.isAftertouch())
        {
            std::cout << "channel "  << outputChannel(msg) << "   " <<
                         "poly-pressure   " << outputNote(msg) << " " << output7Bit(msg.getAfterTouchValue()).paddedLeft(' ', 3) << std::endl;
        }
        else if (msg.isController())
        {
            if (displayControlChange)
            {
                if (displayControlChange14Bit)
                {
                    uint8 msb_cc = msg.getControllerNumber();
                    if (msb_cc >= 32)
                    {
                        msb_cc -= 32;
                    }
                    uint8 lsb_cc = msb_cc + 32;
                    uint8 ch = msg.getChannel() - 1;
                    uint16 v = ((lastCC_[ch][msb_cc] & 0x7f) << 7) | (lastCC_[ch][lsb_cc] & 0x7f);
                    std::cout << "channel "  << outputChannel(msg) << "   " <<
                                 "cc14             " << output7Bit(msb_cc).paddedLeft(' ', 3) << " "
                                                     << output14Bit(v).paddedLeft(' ', 5) << std::endl;
                }
                else
                {
                    std::cout << "channel "  << outputChannel(msg) << "   " <<
                                 "control-change   " << output7Bit(msg.getControllerNumber()).paddedLeft(' ', 3) << "   "
                                                     << output7Bit(msg.getControllerValue()).paddedLeft(' ', 3) << std::endl;
                }
            }
            
            if (displayNrpn)
            {
                std::cout << "channel "  << outputChannel(msg) << "   " <<
                             "nrpn           " << output14Bit(rpnMsg_.parameterNumber).paddedLeft(' ', 5) << " "
                                               << output14Bit(rpnMsg_.value).paddedLeft(' ', 5) << std::endl;
            }
            else if (displayRpn)
            {
                std::cout << "channel "  << outputChannel(msg) << "   " <<
                             "rpn            " << output14Bit(rpnMsg_.parameterNumber).paddedLeft(' ', 5) << " "
                                               << output14Bit(rpnMsg_.value).paddedLeft(' ', 5) << std::endl;
            }
        }
        else if (msg.isProgramChange())
        {
            std::cout << "channel "  << outputChannel(msg) << "   " <<
                         "program-change   " << output7Bit(msg.getProgramChangeNumber()).paddedLeft(' ', 7) << std::endl;
        }
        else if (msg.isChannelPressure())
        {
            std::cout << "channel "  << outputChannel(msg) << "   " <<
                         "channel-pressure " << output7Bit(msg.getChannelPressureValue()).paddedLeft(' ', 7) << std::endl;
        }
        else if (msg.isPitchWheel())
        {
            std::cout << "channel "  << outputChannel(msg) << "   " <<
                         "pitch-bend       " << output14Bit(msg.getPitchWheelValue()).paddedLeft(' ', 7) << std::endl;
        }
        else if (msg.isMidiClock())
        {
            std::cout << "midi-clock" << std::endl;
        }
        else if (msg.isMidiStart())
        {
            std::cout << "start" << std::endl;
        }
        else if (msg.isMidiStop())
        {
            std::cout << "stop" << std::endl;
        }
        else if (msg.isMidiContinue())
        {
            std::cout << "continue" << std::endl;
        }
        else if (msg.isActiveSense())
        {
            std::cout << "active-sensing" << std::endl;
        }
        else if (msg.getRawDataSize() == 1 && msg.getRawData()[0] == 0xff)
        {
            std::cout << "reset" << std::endl;
        }
        else if (msg.isSysEx())
        {
            if (sysexOutput_.get() != nullptr)
            {
                const uint8* data = msg.getRawData();
                const int size = msg.getRawDataSize();
                sysexOutput_->write(data, size);
                sysexOutput_->flush();
                std::cout << "system-exclusive-file " << size << " bytes" << std::endl;
            }
            else
            {
                std::cout << "system-exclusive";
                
                if (!useHexadecimalsByDefault_)
                {
                    std::cout << " hex";
                }
                
                const uint8* data = msg.getSysExData();
                int size = msg.getSysExDataSize();
                while (size--)
                {
                    uint8 b = *data++;
                    std::cout << " " << output7BitAsHex(b);
                }
                
                if (!useHexadecimalsByDefault_)
                {
                    std::cout << " dec" << std::endl;
                }
            }
        }
        else if (msg.isQuarterFrame())
        {
            std::cout << "time-code " << output7Bit(msg.getQuarterFrameSequenceNumber()).paddedLeft(' ', 2) << " " << output7Bit(msg.getQuarterFrameValue()) << std::endl;
        }
        else if (msg.isSongPositionPointer())
        {
            std::cout << "song-position " << output14Bit(msg.getSongPositionPointerMidiBeat()).paddedLeft(' ', 5) << std::endl;
        }
        else if (msg.getRawDataSize() == 2 && msg.getRawData()[0] == 0xf3)
        {
            std::cout << "song-select " << output7Bit(msg.getRawData()[1]).paddedLeft(' ', 3) << std::endl;
        }
        else if (msg.getRawDataSize() == 1 && msg.getRawData()[0] == 0xf6)
        {
            std::cout << "tune-request" << std::endl;
        }
    }
    
    String output7BitAsHex(int v)
    {
        return String::toHexString(v).paddedLeft('0', 2).toUpperCase();
    }
    
    String output7Bit(int v)
    {
        if (useHexadecimalsByDefault_)
        {
            return output7BitAsHex(v);
        }
        else
        {
            return String(v);
        }
    }
    
    String output14BitAsHex(int v)
    {
        return String::toHexString(v).paddedLeft('0', 4).toUpperCase();
    }
    
    String output14Bit(int v)
    {
        if (useHexadecimalsByDefault_)
        {
            return output14BitAsHex(v);
        }
        else
        {
            return String(v);
        }
    }
    
    String outputNote(const MidiMessage& msg)
    {
        if (noteNumbersOutput_)
        {
            return output7Bit(msg.getNoteNumber()).paddedLeft(' ', 4);
        }
        else
        {
            return MidiMessage::getMidiNoteName(msg.getNoteNumber(), true, true, octaveMiddleC_).paddedLeft(' ', 4);
        }
    }
    
    String outputChannel(const MidiMessage& msg)
    {
        return output7Bit(msg.getChannel()).paddedLeft(' ', 2);
    }
    
    bool tryToConnectMidiInput()
    {
        std::unique_ptr<MidiInput> midi_input = nullptr;
        String midi_input_name;
        
        auto devices = MidiInput::getAvailableDevices();
        for (int i = 0; i < devices.size(); ++i)
        {
            if (devices[i].name == midiInName_)
            {
                midi_input = MidiInput::openDevice(devices[i].identifier, this);
                midi_input_name = devices[i].name;
                break;
            }
        }
        
        if (midi_input == nullptr)
        {
            for (int i = 0; i < devices.size(); ++i)
            {
                if (devices[i].name.containsIgnoreCase(midiInName_))
                {
                    midi_input = MidiInput::openDevice(devices[i].identifier, this);
                    midi_input_name = devices[i].name;
                    break;
                }
            }
        }
        
        if (midi_input)
        {
            midi_input->start();
            midiIn_.swap(midi_input);
            fullMidiInName_ = midi_input_name;
            return true;
        }
        
        return false;
    }
    
    void executeCommand(ApplicationCommand& cmd)
    {
        switch (cmd.command_)
        {
            case NONE:
                break;
            case LIST:
                for (auto&& device : MidiInput::getAvailableDevices())
                {
                    std::cout << device.name << std::endl;
                }
                systemRequestedQuit();
                break;
            case DEVICE:
            {
                midiIn_ = nullptr;
                midiInName_ = cmd.opts_[0];
                
                if (!tryToConnectMidiInput())
                {
                    std::cerr << "Couldn't find MIDI input port \"" << midiInName_ << "\", waiting." << std::endl;
                }
                break;
            }
            case VIRTUAL:
            {
#if (JUCE_LINUX || JUCE_MAC)
                String name = DEFAULT_VIRTUAL_NAME;
                if (cmd.opts_.size())
                {
                    name = cmd.opts_[0];
                }
                midiIn_ = MidiInput::createNewDevice(name, this);
                if (midiIn_ == nullptr)
                {
                    std::cerr << "Couldn't create virtual MIDI input port \"" << name << "\"" << std::endl;
                    setApplicationReturnValue(EXIT_FAILURE);
                }
                else
                {
                    midiInName_ = cmd.opts_[0];
                    fullMidiInName_.clear();
                    midiIn_->start();
                }
#else
                std::cerr << "Virtual MIDI input ports are not supported on Windows" << std::endl;
                setApplicationReturnValue(EXIT_FAILURE);
#endif
                break;
            }
            case PASSTHROUGH:
            {
                midiOut_ = nullptr;
                midiOutName_ = cmd.opts_[0];
                auto devices = MidiOutput::getAvailableDevices();
                for (int i = 0; i < devices.size(); ++i)
                {
                    if (devices[i].name == midiOutName_)
                    {
                        midiOut_ = MidiOutput::openDevice(devices[i].identifier);
                        midiOutName_ = devices[i].name;
                        break;
                    }
                }
                
                if (midiOut_ == nullptr)
                {
                    for (int i = 0; i < devices.size(); ++i)
                    {
                        if (devices[i].name.containsIgnoreCase(midiOutName_))
                        {
                            midiOut_ = MidiOutput::openDevice(devices[i].identifier);
                            midiOutName_ = devices[i].name;
                            break;
                        }
                    }
                }
                if (midiOut_ == nullptr)
                {
                    std::cerr << "Couldn't find MIDI output port \"" << midiOutName_ << "\"" << std::endl;
                    setApplicationReturnValue(EXIT_FAILURE);
                }
                break;
            }
            case TXTFILE:
            {
                String path(cmd.opts_[0]);
                File file = File::getCurrentWorkingDirectory().getChildFile(path);
                if (file.existsAsFile())
                {
                    parseFile(file);
                }
                else
                {
                    std::cerr << "Couldn't find file \"" << path << "\"" << std::endl;
                    setApplicationReturnValue(EXIT_FAILURE);
                }
                break;
            }
            case DECIMAL:
            case HEXADECIMAL:
                // these are not commands but rather configuration options
                // allow them to be inlined anywhere by handling them immediately in the
                // parseParameters method
                break;
            case TIMESTAMP:
                timestampOutput_ = true;
                break;
            case NOTE_NUMBERS:
                noteNumbersOutput_ = true;
                break;
            case OCTAVE_MIDDLE_C:
                octaveMiddleC_ = asDecOrHex7BitValue(cmd.opts_[0]);
                break;
            case QUIET:
                quiet_ = true;
                break;
            case RAWDUMP:
                rawdump_ = true;
                break;
            case JAVASCRIPT:
                scriptCode_ = cmd.opts_[0];
                break;
            case JAVASCRIPT_FILE:
            {
                String path(cmd.opts_[0]);
                File file = File::getCurrentWorkingDirectory().getChildFile(path);
                if (file.existsAsFile())
                {
                    scriptCode_ = file.loadFileAsString();
                }
                else
                {
                    std::cerr << "Couldn't find file \"" << path << "\"" << std::endl;
                    setApplicationReturnValue(EXIT_FAILURE);
                }
                break;
            }
            case SYSTEM_EXCLUSIVE_FILE:
            {
                String path(cmd.opts_[0]);
                File file = File::getCurrentWorkingDirectory().getChildFile(path);
                file.deleteFile();
                sysexOutput_ = file.createOutputStream();
                if (sysexOutput_.get() == nullptr)
                {
                    std::cerr << "Couldn't create file \"" << path << "\"" << std::endl;
                    setApplicationReturnValue(EXIT_FAILURE);
                }
                else
                {
                    filterCommands_.add(cmd);
                }
                break;
            }
            default:
                filterCommands_.add(cmd);
                break;
        }
    }
    
    uint8 asNoteNumber(String value)
    {
        if (value.length() >= 2)
        {
            value = value.toUpperCase();
            String first = value.substring(0, 1);
            if (first.containsOnly("CDEFGABH") && value.substring(value.length()-1).containsOnly("1234567890"))
            {
                int note = 0;
                switch (first[0])
                {
                    case 'C': note = 0; break;
                    case 'D': note = 2; break;
                    case 'E': note = 4; break;
                    case 'F': note = 5; break;
                    case 'G': note = 7; break;
                    case 'A': note = 9; break;
                    case 'B': note = 11; break;
                    case 'H': note = 11; break;
                }
                
                if (value[1] == 'B')
                {
                    note -= 1;
                }
                else if (value[1] == '#')
                {
                    note += 1;
                }
                
                note += (value.getTrailingIntValue() + 5 - octaveMiddleC_) * 12;
                
                return (uint8)limit7Bit(note);
            }
        }
        
        return (uint8)limit7Bit(asDecOrHexIntValue(value));
    }
    
    uint8 asDecOrHex7BitValue(String value)
    {
        return (uint8)limit7Bit(asDecOrHexIntValue(value));
    }
    
    uint16 asDecOrHex14BitValue(String value)
    {
        return (uint16)limit14Bit(asDecOrHexIntValue(value));
    }
    
    int asDecOrHexIntValue(String value)
    {
        if (value.endsWithIgnoreCase("H"))
        {
            return value.dropLastCharacters(1).getHexValue32();
        }
        else if (value.endsWithIgnoreCase("M"))
        {
            return value.getIntValue();
        }
        else if (useHexadecimalsByDefault_)
        {
            return value.getHexValue32();
        }
        else
        {
            return value.getIntValue();
        }
    }
    
    static uint8 limit7Bit(int value)
    {
        return (uint8)jlimit(0, 0x7f, value);
    }
    
    static uint16 limit14Bit(int value)
    {
        return (uint16)jlimit(0, 0x3fff, value);
    }
    
    void printVersion()
    {
        std::cout << ProjectInfo::projectName << " v" << ProjectInfo::versionString << std::endl;
        std::cout << "https://github.com/gbevin/ReceiveMIDI" << std::endl;
    }
    
    void printUsage()
    {
        printVersion();
        std::cout << std::endl;
        std::cout << "Usage: " << ProjectInfo::projectName << " [ commands ] [ programfile ] [ -- ]" << std::endl << std::endl
                  << "Commands:" << std::endl;
        for (auto&& cmd : commands_)
        {
            std::cout << "  " << cmd.param_.paddedRight(' ', 5);
            if (cmd.optionsDescription_.isNotEmpty())
            {
                std::cout << " " << cmd.optionsDescription_.paddedRight(' ', 9);
            }
            else
            {
                std::cout << "          ";
            }
            std::cout << "  " << cmd.commandDescription_;
            std::cout << std::endl;
        }
        std::cout << "  -h  or  --help   Print Help (this message) and exit" << std::endl;
        std::cout << "  --version        Print version information and exit" << std::endl;
        std::cout << "  --               Read commands from standard input until it's closed" << std::endl;
        std::cout << std::endl;
        std::cout << "Alternatively, you can use the following long versions of the commands:" << std::endl;
        String line = " ";
        for (auto&& cmd : commands_)
        {
            if (cmd.altParam_.isNotEmpty())
            {
                if (line.length() + cmd.altParam_.length() + 1 >= 80)
                {
                    std::cout << line << std::endl;
                    line = " ";
                }
                line << " " << cmd.altParam_;
            }
        }
        std::cout << line << std::endl << std::endl;
        std::cout << "By default, numbers are interpreted in the decimal system, this can be changed" << std::endl
                  << "to hexadecimal by sending the \"hex\" command. Additionally, by suffixing a" << std::endl
                  << "number with \"M\" or \"H\", it will be interpreted as a decimal or hexadecimal" << std::endl
                  << "respectively." << std::endl;
        std::cout << std::endl;
        std::cout << "The MIDI device name doesn't have to be an exact match." << std::endl;
        std::cout << "If ReceiveMIDI can't find the exact name that was specified, it will pick the" << std::endl
                  << "first MIDI output port that contains the provided text, irrespective of case." << std::endl;
        std::cout << std::endl;
        std::cout << "Where notes can be provided as arguments, they can also be written as note" << std::endl
                  << "names, by default from C-2 to G8 which corresponds to note numbers 0 to 127." << std::endl
                  << "By setting the octave for middle C, the note name range can be changed." << std::endl
                  << "Sharps can be added by using the \"#\" symbol after the note letter, and flats" << std::endl
                  << "by using the letter \"b\"." << std::endl;
        std::cout << std::endl;
        std::cout << "For details on how to use the \"javascript\" and \"javascript-file\" commands," << std::endl
                  << "please refer to the JAVASCRIPT.md documentation file." << std::endl;
    }
    
    Array<ApplicationCommand> commands_;
    Array<ApplicationCommand> filterCommands_;
    bool timestampOutput_;
    bool noteNumbersOutput_;
    int octaveMiddleC_;
    bool useHexadecimalsByDefault_;
    bool quiet_;
    bool rawdump_;
    String midiInName_;
    std::unique_ptr<MidiInput> midiIn_;
    String fullMidiInName_;
    String midiOutName_;
    std::unique_ptr<MidiOutput> midiOut_;
    ApplicationCommand currentCommand_;
    JavascriptEngine scriptEngine_;
    String scriptCode_;
    ScriptMidiMessageClass* scriptMidiMessage_;
    RPNDetector rpnDetector_;
    RPNMessage rpnMsg_;
    uint8 lastCC_[16][128];
    std::unique_ptr<FileOutputStream> sysexOutput_;
};

START_JUCE_APPLICATION (receiveMidiApplication)
