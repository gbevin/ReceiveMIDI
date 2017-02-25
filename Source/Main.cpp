/*
 * This file is part of ReceiveMIDI.
 * Copyright (command) 2017 Uwyn SPRL.  http://www.uwyn.com
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

#include <sstream>

enum CommandIndex
{
    NONE,
    LIST,
    DEVICE,
    TXTFILE,
    DECIMAL,
    HEXADECIMAL,
    CHANNEL,
    TIMESTAMP,
    VOICE,
    NOTE,
    NOTE_ON,
    NOTE_OFF,
    POLY_PRESSURE,
    CONTROL_CHANGE,
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
    SYSTEM_COMMON,
    TIME_CODE,
    SONG_POSITION,
    SONG_SELECT,
    TUNE_REQUEST
};

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
        commands_.add({"dev",   "device",           DEVICE,             1, "name",           "Set the name of the MIDI input port (REQUIRED)"});
        commands_.add({"list",  "",                 LIST,               0, "",               "Lists the MIDI input ports"});
        commands_.add({"file",  "",                 TXTFILE,            1, "path",           "Loads commands from the specified program file"});
        commands_.add({"dec",   "decimal",          DECIMAL,            0, "",               "Interpret the next numbers as decimals by default"});
        commands_.add({"hex",   "hexadecimal",      HEXADECIMAL,        0, "",               "Interpret the next numbers as hexadecimals by default"});
        commands_.add({"ch",    "channel",          CHANNEL,            1, "number",         "Set MIDI channel for the commands (0-16), defaults to 0"});
        commands_.add({"ts",    "timestamp",        TIMESTAMP,          0, "",               "Output a timestamp for each receive MIDI message"});
        commands_.add({"voice", "",                 VOICE,              0, "",               "Show all Channel Voice messages"});
        commands_.add({"note",  "",                 NOTE,               0, "",               "Show all Note messages"});
        commands_.add({"on",    "note-on",          NOTE_ON,           -1, "(note)",         "Show Note On, optionally for note (0-127)"});
        commands_.add({"off",   "note-off",         NOTE_OFF,          -1, "(note)",         "Show Note Off, optionally for note (0-127)"});
        commands_.add({"pp",    "poly-pressure",    POLY_PRESSURE,     -1, "(note)",         "Show Poly Pressure, optionally for note (0-127)"});
        commands_.add({"cc",    "control-change",   CONTROL_CHANGE,    -1, "(number)",       "Show Control Change, optionally for controller (0-127)"});
        commands_.add({"pc",    "program-change",   PROGRAM_CHANGE,    -1, "(number)",       "Show Program Change, optionally for program (0-127)"});
        commands_.add({"cp",    "channel-pressure", CHANNEL_PRESSURE,   0, "",               "Show Channel Pressure"});
        commands_.add({"pb",    "pitch-bend",       PITCH_BEND,         0, "",               "Show Pitch Bend"});
        commands_.add({"sr",    "system-realtime",  SYSTEM_REALTIME,    0, "",               "Show all System Real-Time messages"});
        commands_.add({"clock", "",                 CLOCK,              0, "",               "Show Timing Clock"});
        commands_.add({"start", "",                 START,              0, "",               "Show Start"});
        commands_.add({"stop",  "",                 STOP,               0, "",               "Show Stop"});
        commands_.add({"cont",  "continue",         CONTINUE,           0, "",               "Show Continue"});
        commands_.add({"as",    "active-sensing",   ACTIVE_SENSING,     0, "",               "Show Active Sensing"});
        commands_.add({"rst",   "reset",            RESET,              0, "",               "Show Reset"});
        commands_.add({"sc",    "system-common",    SYSTEM_COMMON,      0, "",               "Show all System Common messages"});
        commands_.add({"syx",   "system-exclusive", SYSTEM_EXCLUSIVE,   0, "",               "Show System Exclusive"});
        commands_.add({"tc",    "time-code",        TIME_CODE,          0, "",               "Show MIDI Time Code Quarter Frame"});
        commands_.add({"spp",   "song-position",    SONG_POSITION,      0, "",               "Show Song Position Pointer"});
        commands_.add({"ss",    "song-select",      SONG_SELECT,        0, "",               "Show Song Select"});
        commands_.add({"tun",   "tune-request",     TUNE_REQUEST,       0, "",               "Show Tune Request"});
        
        timestampOutput_ = false;
        useHexadecimalsByDefault_ = false;
        currentCommand_ = ApplicationCommand::Dummy();
    }
    
    const String getApplicationName() override       { return ProjectInfo::projectName; }
    const String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override       { return true; }
    void systemRequestedQuit() override              { quit(); }
    
    void initialise(const String&) override
    {
        StringArray cmdLineParams(getCommandLineParameterArray());
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
    
    void timerCallback() override
    {
        if (fullMidiInName_.isNotEmpty() && !MidiInput::getDevices().contains(fullMidiInName_))
        {
            std::cerr << "MIDI input port \"" << fullMidiInName_ << "\" got disconnected, waiting." << std::endl;
            
            fullMidiInName_ = String::empty;
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
    
    void shutdown() override {}
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
    
    void parseParameters(StringArray& parameters)
    {
        for (String param : parameters)
        {
            ApplicationCommand* cmd = findApplicationCommand(param);
            if (cmd)
            {
                // handle configuration commands immediately without setting up a new
                switch (cmd->command_)
                {
                    case DECIMAL:
                        useHexadecimalsByDefault_ = false;
                        break;
                    case HEXADECIMAL:
                        useHexadecimalsByDefault_ = true;
                        break;
                    default:
                        // handle variable arg commands
                        if (currentCommand_.expectedOptions_ < 0)
                        {
                            executeCommand(currentCommand_);
                        }
                        
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
                executeCommand(currentCommand_);
            }
        }
        
        // handle variable arg commands
        if (currentCommand_.expectedOptions_ < 0)
        {
            executeCommand(currentCommand_);
        }
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
    
    void handleIncomingMidiMessage(MidiInput* source, const MidiMessage& msg) override
    {
        if (!filterCommands_.isEmpty())
        {
            bool filtered = false;
            int channel = 0;
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
                                    (cmd.opts_.isEmpty() || (msg.getNoteNumber() == asDecOrHex7BitValue(cmd.opts_[0])));
                        break;
                    case NOTE_OFF:
                        filtered |= checkChannel(msg, channel) &&
                                    msg.isNoteOff() &&
                                    (cmd.opts_.isEmpty() || (msg.getNoteNumber() == asDecOrHex7BitValue(cmd.opts_[0])));
                        break;
                    case POLY_PRESSURE:
                        filtered |= checkChannel(msg, channel) &&
                                    msg.isAftertouch() &&
                                    (cmd.opts_.isEmpty() || (msg.getNoteNumber() == asDecOrHex7BitValue(cmd.opts_[0])));
                        break;
                    case CONTROL_CHANGE:
                        filtered |= checkChannel(msg, channel) &&
                                    msg.isController() &&
                                    (cmd.opts_.isEmpty() || (msg.getControllerNumber() == asDecOrHex7BitValue(cmd.opts_[0])));
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
            std::cout << "channel "  << String(msg.getChannel()).paddedLeft(' ', 2) << "   " <<
                         "note-on          " << String(msg.getNoteNumber()).paddedLeft(' ', 3) << " "
                                             << String(msg.getVelocity()).paddedLeft(' ', 3) << std::endl;
        }
        else if (msg.isNoteOff())
        {
            std::cout << "channel "  << String(msg.getChannel()).paddedLeft(' ', 2) << "   " <<
                         "note-off         " << String(msg.getNoteNumber()).paddedLeft(' ', 3) << " "
                                             << String(msg.getVelocity()).paddedLeft(' ', 3) << std::endl;
        }
        else if (msg.isAftertouch())
        {
            std::cout << "channel "  << String(msg.getChannel()).paddedLeft(' ', 2) << "   " <<
                         "poly-pressure    " << String(msg.getNoteNumber()).paddedLeft(' ', 3) << " "
                                             << String(msg.getAfterTouchValue()).paddedLeft(' ', 3) << std::endl;
        }
        else if (msg.isController())
        {
            std::cout << "channel "  << String(msg.getChannel()).paddedLeft(' ', 2) << "   " <<
                         "control-change   " << String(msg.getControllerNumber()).paddedLeft(' ', 3) << " "
                                             << String(msg.getControllerValue()).paddedLeft(' ', 3) << std::endl;
        }
        else if (msg.isProgramChange())
        {
            std::cout << "channel "  << String(msg.getChannel()).paddedLeft(' ', 2) << "   " <<
                         "program-change   " << String(msg.getProgramChangeNumber()).paddedLeft(' ', 7) << std::endl;
        }
        else if (msg.isChannelPressure())
        {
            std::cout << "channel "  << String(msg.getChannel()).paddedLeft(' ', 2) << "   " <<
                         "channel-pressure " << String(msg.getChannelPressureValue()).paddedLeft(' ', 7) << std::endl;
        }
        else if (msg.isPitchWheel())
        {
            std::cout << "channel "  << String(msg.getChannel()).paddedLeft(' ', 2) << "   " <<
                         "pitch-bend       " << String(msg.getPitchWheelValue()).paddedLeft(' ', 7) << std::endl;
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
            std::cout << "system-exclusive hex";
            int size = msg.getSysExDataSize();
            const uint8* data = msg.getSysExData();
            while (size--)
            {
                uint8 b = *data++;
                std::cout << " " << String::toHexString(b).paddedLeft('0', 2).toUpperCase();
            }
            std::cout << " dec" << std::endl;
        }
        else if (msg.isQuarterFrame())
        {
            std::cout << "time-code " << String(msg.getQuarterFrameSequenceNumber()).paddedLeft(' ', 2) << " " << String(msg.getQuarterFrameValue()) << std::endl;
        }
        else if (msg.isSongPositionPointer())
        {
            std::cout << "song-position " << String(msg.getSongPositionPointerMidiBeat()).paddedLeft(' ', 5) << std::endl;
        }
        else if (msg.getRawDataSize() == 2 && msg.getRawData()[0] == 0xf3)
        {
            std::cout << "song-select " << String(msg.getRawData()[1]).paddedLeft(' ', 3) << std::endl;
        }
        else if (msg.getRawDataSize() == 1 && msg.getRawData()[0] == 0xf6)
        {
            std::cout << "tune-request" << std::endl;
        }
    }
    
    bool tryToConnectMidiInput()
    {
        MidiInput* midi_input = nullptr;
        String midi_input_name;
        
        int index = MidiInput::getDevices().indexOf(midiInName_);
        if (index >= 0)
        {
            midi_input = MidiInput::openDevice(index, this);
            midi_input_name = midiInName_;
        }
        else
        {
            StringArray devices = MidiInput::getDevices();
            for (int i = 0; i < devices.size(); ++i)
            {
                if (devices[i].containsIgnoreCase(midiInName_))
                {
                    midi_input = MidiInput::openDevice(i, this);
                    midi_input_name = devices[i];
                    break;
                }
            }
        }
        
        if (midi_input)
        {
            midi_input->start();
            midiIn_ = midi_input;
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
                for (auto&& device : MidiInput::getDevices())
                {
                    std::cout << device << std::endl;
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
            default:
                filterCommands_.add(cmd);
                break;
        }
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
    
    void printUsage()
    {
        std::cout << ProjectInfo::projectName << " v" << ProjectInfo::versionString << std::endl;
        std::cout << "https://github.com/gbevin/ReceiveMIDI" << std::endl << std::endl;
        std::cout << "Usage: " << ProjectInfo::projectName << " [ commands ] [ programfile ] [ -- ]" << std::endl << std::endl
                  << "Commands:" << std::endl;
        for (auto&& cmd : commands_)
        {
            std::cout << "  " << cmd.param_.paddedRight(' ', 5);
            if (cmd.optionsDescription_.isNotEmpty())
            {
                std::cout << " " << cmd.optionsDescription_.paddedRight(' ', 13);
            }
            else
            {
                std::cout << "              ";
            }
            std::cout << "  " << cmd.commandDescription_;
            std::cout << std::endl;
        }
        std::cout << "  --                   Read commands from standard input until it's closed" << std::endl;
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
                  << "to hexadecimal by sending the \"hex\" command. Additionally, by suffixing a " << std::endl
                  << "number with \"M\" or \"H\", it will be interpreted as a decimal or hexadecimal" << std::endl
                  << "respectively." << std::endl;
        std::cout << std::endl;
        std::cout << "The MIDI device name doesn't have to be an exact match." << std::endl;
        std::cout << "If ReceiveMIDI can't find the exact name that was specified, it will pick the" << std::endl
                  << "first MIDI output port that contains the provided text, irrespective of case." << std::endl;
        std::cout << std::endl;
    }
    
    Array<ApplicationCommand> commands_;
    Array<ApplicationCommand> filterCommands_;
    bool timestampOutput_;
    String midiInName_;
    ScopedPointer<MidiInput> midiIn_;
    String fullMidiInName_;
    bool useHexadecimalsByDefault_;
    ApplicationCommand currentCommand_;
};

START_JUCE_APPLICATION (receiveMidiApplication)
