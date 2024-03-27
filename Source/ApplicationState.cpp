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

#include "ApplicationState.h"

#include "ScriptOscClass.h"
#include "ScriptUtilClass.h"

static const int DEFAULT_OCTAVE_MIDDLE_C = 3;
static const String& DEFAULT_VIRTUAL_NAME = "ReceiveMIDI";

inline float sign(float value)
{
    return (float)(value > 0.) - (value < 0.);
}

ApplicationState::ApplicationState()
{
    commands_.add({"dev",   "device",                   DEVICE,                1, {"name"},             {"Set the name of the MIDI input port"}});
    commands_.add({"virt",  "virtual",                  VIRTUAL,              -1, {"(name)"},           {"Use virtual MIDI port with optional name (Linux/macOS)"}});
    commands_.add({"pass",  "pass-through",             PASSTHROUGH,           1, {"name"},             {"Set name of MIDI output port for MIDI pass-through"}});
    commands_.add({"list",  "",                         LIST,                  0, {""},                 {"Lists the MIDI input ports"}});
    commands_.add({"file",  "",                         TXTFILE,               1, {"path"},             {"Loads commands from the specified program file"}});
    commands_.add({"dec",   "decimal",                  DECIMAL,               0, {""},                 {"Interpret the next numbers as decimals by default"}});
    commands_.add({"hex",   "hexadecimal",              HEXADECIMAL,           0, {""},                 {"Interpret the next numbers as hexadecimals by default"}});
    commands_.add({"ch",    "channel",                  CHANNEL,               1, {"number"},           {"Set MIDI channel for the commands (0-16), defaults to 0"}});
    commands_.add({"ts",    "timestamp",                TIMESTAMP,             0, {""},                 {"Output a timestamp for each received MIDI message"}});
    commands_.add({"nn",    "note-numbers",             NOTE_NUMBERS,          0, {""},                 {"Output notes as numbers instead of names"}});
    commands_.add({"omc",   "octave-middle-c",          OCTAVE_MIDDLE_C,       1, {"number"},           {"Set octave for middle C, defaults to 3"}});
    commands_.add({"voice", "",                         VOICE,                 0, {""},                 {"Show all Channel Voice messages"}});
    commands_.add({"note",  "",                         NOTE,                  0, {""},                 {"Show all Note messages"}});
    commands_.add({"on",    "note-on",                  NOTE_ON,              -1, {"(note)"},           {"Show Note On, optionally for note (0-127)"}});
    commands_.add({"off",   "note-off",                 NOTE_OFF,             -1, {"(note)"},           {"Show Note Off, optionally for note (0-127)"}});
    commands_.add({"pp",    "poly-pressure",            POLY_PRESSURE,        -1, {"(note)"},           {"Show Poly Pressure, optionally for note (0-127)"}});
    commands_.add({"cc",    "control-change",           CONTROL_CHANGE,       -1, {"(number)"},         {"Show Control Change, optionally for controller (0-127)"}});
    commands_.add({"cc14",  "control-change-14",        CONTROL_CHANGE_14BIT, -1, {"(number)"},         {"Show 14-bit CC, optionally for controller (0-63)"}});
    commands_.add({"nrpn",  "",                         NRPN,                 -1, {"(number)"},         {"Show NRPN, optionally for parameter (0-16383)"}});
    commands_.add({"nrpnf", "nrpn-full",                NRPN_FULL,            -1, {"(number)"},         {"Show full NRPN (MSB+LSB), optionally for parameter (0-16383)"}});
    commands_.add({"rpn",   "",                         RPN,                  -1, {"(number)"},         {"Show RPN, optionally for parameter (0-16383)"}});
    commands_.add({"rpnf",  "rpn-full",                 RPN_FULL,             -1, {"(number)"},         {"Show full RPN (MSB+LSB), optionally for parameter (0-16383)"}});
    commands_.add({"pc",    "program-change",           PROGRAM_CHANGE,       -1, {"(number)"},         {"Show Program Change, optionally for program (0-127)"}});
    commands_.add({"cp",    "channel-pressure",         CHANNEL_PRESSURE,      0, {""},                 {"Show Channel Pressure"}});
    commands_.add({"pb",    "pitch-bend",               PITCH_BEND,            0, {""},                 {"Show Pitch Bend"}});
    commands_.add({"sr",    "system-realtime",          SYSTEM_REALTIME,       0, {""},                 {"Show all System Real-Time messages"}});
    commands_.add({"clock", "",                         CLOCK,                 0, {""},                 {"Show Timing Clock"}});
    commands_.add({"start", "",                         START,                 0, {""},                 {"Show Start"}});
    commands_.add({"stop",  "",                         STOP,                  0, {""},                 {"Show Stop"}});
    commands_.add({"cont",  "continue",                 CONTINUE,              0, {""},                 {"Show Continue"}});
    commands_.add({"as",    "active-sensing",           ACTIVE_SENSING,        0, {""},                 {"Show Active Sensing"}});
    commands_.add({"rst",   "reset",                    RESET,                 0, {""},                 {"Show Reset"}});
    commands_.add({"sc",    "system-common",            SYSTEM_COMMON,         0, {""},                 {"Show all System Common messages"}});
    commands_.add({"syx",   "system-exclusive",         SYSTEM_EXCLUSIVE,      0, {""},                 {"Show System Exclusive"}});
    commands_.add({"syf",   "system-exclusive-file",    SYSTEM_EXCLUSIVE_FILE, 1, {"path"},             {"Store SysEx into a .syx file"}});
    commands_.add({"tc",    "time-code",                TIME_CODE,             0, {""},                 {"Show MIDI Time Code Quarter Frame"}});
    commands_.add({"spp",   "song-position",            SONG_POSITION,         0, {""},                 {"Show Song Position Pointer"}});
    commands_.add({"ss",    "song-select",              SONG_SELECT,           0, {""},                 {"Show Song Select"}});
    commands_.add({"tun",   "tune-request",             TUNE_REQUEST,          0, {""},                 {"Show Tune Request"}});
    commands_.add({"q",     "quiet",                    QUIET,                 0, {""},                 {"Don't show the received messages on standard output"}});
    commands_.add({"dump",  "",                         RAWDUMP,               0, {""},                 {"Dump the received messages 1:1 on standard output"}});
    commands_.add({"js",    "javascript",               JAVASCRIPT,            1, {"code"},             {"Execute this script for each received MIDI message"}});
    commands_.add({"jsf",   "javascript-file",          JAVASCRIPT_FILE,       1, {"path"},             {"Execute the script in this file for each message"}});
    commands_.add({"mpp",   "mpe-profile",              MPE_PROFILE,           3, {"name", "manager", "members"},
                                                                                  {"Configure a responder MPE Profile creating virtual MIDI input",
                                                                                   "and output ports with the provided name, available manager",
                                                                                   "channel (1-15 or 0 for any) and desired member channel",
                                                                                   "count (1-15, or 0 for any) (Linux/macOS)"}});

    timestampOutput_ = false;
    noteNumbersOutput_ = false;
    octaveMiddleC_ = DEFAULT_OCTAVE_MIDDLE_C;
    useHexadecimalsByDefault_ = false;
    quiet_ = false;
    rawdump_ = false;
    currentCommand_ = ApplicationCommand::Dummy();
    
    mpeProfile_ = std::make_unique<MpeProfileNegotiation>(this);
    
    // initialize last CC MSB values
    for (int ch = 0; ch < 16; ++ch)
    {
        for (int cc = 0; cc < 128; ++cc)
        {
            lastCC_[ch][cc] = -1;
        }
    }
}

void ApplicationState::initialise(JUCEApplicationBase& app)
{
    StringArray cmdLineParams(app.getCommandLineParameterArray());
    if (cmdLineParams.contains("--help") || cmdLineParams.contains("-h"))
    {
        printUsage();
        app.systemRequestedQuit();
        return;
    }
    else if (cmdLineParams.contains("--version"))
    {
        printVersion();
        app.systemRequestedQuit();
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
        app.systemRequestedQuit();
    }
    else
    {
        startTimer(200);
    }
}

bool ApplicationState::isMidiInDeviceAvailable(const String& name)
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

void ApplicationState::timerCallback()
{
    if (fullMidiInName_.isNotEmpty() && !isMidiInDeviceAvailable(fullMidiInName_))
    {
        std::cerr << "MIDI input port \"" << fullMidiInName_ << "\" got disconnected, waiting" << std::endl;
        
        fullMidiInName_ = String();
        midiIn_ = nullptr;
    }
    else if ((midiInName_.isNotEmpty() && midiIn_ == nullptr))
    {
        if (tryToConnectMidiInput())
        {
            std::cerr << "Connected to MIDI input port \"" << fullMidiInName_ << "\"" << std::endl;
        }
    }
}

void ApplicationState::shutdown()
{
    if (sysexOutput_)
    {
        sysexOutput_->flush();
        sysexOutput_.reset();
    }
}

ApplicationCommand* ApplicationState::findApplicationCommand(const String& param)
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

StringArray ApplicationState::parseLineAsParameters(const String& line)
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

void ApplicationState::executeCurrentCommand()
{
    ApplicationCommand cmd = currentCommand_;
    currentCommand_ = ApplicationCommand::Dummy();
    executeCommand(cmd);
}

void ApplicationState::handleVarArgCommand()
{
    if (currentCommand_.expectedOptions_ < 0)
    {
        executeCurrentCommand();
    }
}

void ApplicationState::openInputDevice(const String& name)
{
    midiIn_ = nullptr;
    midiInName_ = name;
    
    if (!tryToConnectMidiInput())
    {
        std::cerr << "Couldn't find MIDI input port \"" << name << "\", waiting" << std::endl;
    }
}

std::unique_ptr<MidiOutput> ApplicationState::openOutputDevice(const String& name)
{
    std::unique_ptr<MidiOutput> output;

    String output_name = name;
    
    auto devices = MidiOutput::getAvailableDevices();
    for (int i = 0; i < devices.size(); ++i)
    {
        if (devices[i].name == output_name)
        {
            output = MidiOutput::openDevice(devices[i].identifier);
            output_name = devices[i].name;
            break;
        }
    }
    
    if (output == nullptr)
    {
        for (int i = 0; i < devices.size(); ++i)
        {
            if (devices[i].name.containsIgnoreCase(output_name))
            {
                output = MidiOutput::openDevice(devices[i].identifier);
                output_name = devices[i].name;
                break;
            }
        }
    }
    if (output == nullptr)
    {
        std::cerr << "Couldn't find MIDI output port \"" << output_name << "\"" << std::endl;
        JUCEApplicationBase::getInstance()->setApplicationReturnValue(EXIT_FAILURE);
    }
    
    return output;
}

void ApplicationState::parseParameters(StringArray& parameters)
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

void ApplicationState::parseFile(File file)
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

void ApplicationState::handleIncomingMidiMessage(MidiInput*, const MidiMessage& msg)
{
    DisplayState display;
    
    if (!filterCommands_.isEmpty())
    {
        display.filtered = false;
        display.channel = 0;
        display.displayControlChange = false;
        display.displayControlChange14bit = false;
        
        for (ApplicationCommand& cmd : filterCommands_)
        {
            cmd.filter(*this, msg, display);
        }
        
        if (!display.filtered)
        {
            return;
        }
    }
    
    if (midiPass_)
    {
        midiPass_->sendMessageNow(msg);
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
            outputMessage(msg, display);
        }
    }
}

void ApplicationState::dumpMessage(const MidiMessage& msg)
{
    std::cout.write((char *)msg.getRawData(), msg.getRawDataSize());
    std::cout.flush();
}

void ApplicationState::outputMessage(const MidiMessage& msg, DisplayState display)
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
        if (display.displayControlChange)
        {
            if (display.displayControlChange14bit)
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
        
        if (display.displayNrpn)
        {
            std::cout << "channel "  << outputChannel(msg) << "   " <<
                         "nrpn           " << output14Bit(rpnMsg_.parameterNumber).paddedLeft(' ', 5) << " "
            << output14Bit(rpnMsg_.value).paddedLeft(' ', 5) << std::endl;
        }
        else if (display.displayRpn)
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

String ApplicationState::output7BitAsHex(int v)
{
    return String::toHexString(v).paddedLeft('0', 2).toUpperCase();
}

String ApplicationState::output7Bit(int v)
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

String ApplicationState::output14BitAsHex(int v)
{
    return String::toHexString(v).paddedLeft('0', 4).toUpperCase();
}

String ApplicationState::output14Bit(int v)
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

String ApplicationState::outputNote(const MidiMessage& msg)
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

String ApplicationState::outputChannel(const MidiMessage& msg)
{
    return output7Bit(msg.getChannel()).paddedLeft(' ', 2);
}

bool ApplicationState::tryToConnectMidiInput()
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

void ApplicationState::executeCommand(ApplicationCommand& cmd)
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
            JUCEApplicationBase::getInstance()->systemRequestedQuit();
            break;
        case DEVICE:
        {
            openInputDevice(cmd.opts_[0]);
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
                JUCEApplicationBase::getInstance()->setApplicationReturnValue(EXIT_FAILURE);
            }
            else
            {
                midiInName_ = cmd.opts_[0];
                fullMidiInName_.clear();
                midiIn_->start();
            }
#else
            std::cerr << "Virtual MIDI input ports are not supported on Windows" << std::endl;
            JUCEApplicationBase::getInstance()->setApplicationReturnValue(EXIT_FAILURE);
#endif
            break;
        }
        case PASSTHROUGH:
        {
            midiPass_ = openOutputDevice(cmd.opts_[0]);
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
                JUCEApplicationBase::getInstance()->setApplicationReturnValue(EXIT_FAILURE);
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
                JUCEApplicationBase::getInstance()->setApplicationReturnValue(EXIT_FAILURE);
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
                JUCEApplicationBase::getInstance()->setApplicationReturnValue(EXIT_FAILURE);
            }
            else
            {
                filterCommands_.add(cmd);
            }
            break;
        }
        case MPE_PROFILE:
        {
#if (JUCE_LINUX || JUCE_MAC)
            mpeProfile_->setManager(jlimit(0, 15, asDecOrHexIntValue(cmd.opts_[1])));
            mpeProfile_->setMembers(jlimit(0, 15, asDecOrHexIntValue(cmd.opts_[2])));
            mpeProfile_->setProfileMidiName(cmd.opts_[0]);
#else
            std::cerr << "MPE Profile responder with virtual MIDI ports is not supported on Windows" << std::endl;
            setApplicationReturnValue(EXIT_FAILURE);
#endif
            break;
        }
        default:
            filterCommands_.add(cmd);
            break;
    }
}

uint8 ApplicationState::asNoteNumber(String value)
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

uint8 ApplicationState::asDecOrHex7BitValue(String value)
{
    return (uint8)limit7Bit(asDecOrHexIntValue(value));
}

uint16 ApplicationState::asDecOrHex14BitValue(String value)
{
    return (uint16)limit14Bit(asDecOrHexIntValue(value));
}

int ApplicationState::asDecOrHexIntValue(String value)
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

uint8 ApplicationState::limit7Bit(int value)
{
    return (uint8)jlimit(0, 0x7f, value);
}

uint16 ApplicationState::limit14Bit(int value)
{
    return (uint16)jlimit(0, 0x3fff, value);
}

void ApplicationState::printVersion()
{
    std::cout << ProjectInfo::projectName << " v" << ProjectInfo::versionString << std::endl;
    std::cout << "https://github.com/gbevin/ReceiveMIDI" << std::endl;
}

void ApplicationState::printUsage()
{
    printVersion();
    std::cout << std::endl;
    std::cout << "Usage: " << ProjectInfo::projectName << " [ commands ] [ programfile ] [ -- ]" << std::endl << std::endl
    << "Commands:" << std::endl;
    for (auto&& cmd : commands_)
    {
        String param_option;
        param_option << "  " << cmd.param_.paddedRight(' ', 5);
        if (!cmd.optionsDescriptions_.isEmpty())
        {
            param_option << " " << cmd.optionsDescriptions_.getReference(0).paddedRight(' ', 9);
        }
        else
        {
            param_option << "              ";
        }
        param_option << "  ";
        param_option = param_option.substring(0, 19);
        std::cout << param_option;
        if (!cmd.commandDescriptions_.isEmpty())
        {
            std::cout << cmd.commandDescriptions_.getReference(0);
        }
        std::cout << std::endl;
        
        if (cmd.optionsDescriptions_.size() > 1)
        {
            auto i = 1;
            for (; i < cmd.optionsDescriptions_.size(); ++i)
            {
                auto line = cmd.optionsDescriptions_.getReference(i);
                String param_option;
                param_option << "        " << line.paddedRight(' ', 9) << "  ";
                param_option = param_option.substring(0, 19);
                std::cout << param_option;
                
                if (i < cmd.commandDescriptions_.size())
                {
                    std::cout << cmd.commandDescriptions_.getReference(i);
                }
                
                std::cout << std::endl;
            }
            for (; i < cmd.commandDescriptions_.size(); ++i)
            {
                std::cout << "                   " << cmd.commandDescriptions_.getReference(i) << std::endl;
            }
        }
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
