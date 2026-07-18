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
#include "TerminalColor.h"

#include <algorithm>
#include <cmath>
#include <sstream>

static const int DEFAULT_OCTAVE_MIDDLE_C = 3;
static const String& DEFAULT_VIRTUAL_NAME = "ReceiveMIDI";

// Optional ANSI color for the help text. It is emitted only when standard output
// is an interactive terminal that is expected to understand the codes (see
// TerminalColor.cpp), so any redirected or piped output stays plain and
// byte-for-byte as before.
namespace ansi
{
    static const char* const reset = "\x1b[0m";

    // Each role carries a 24-bit truecolor code matching the palette on the
    // uwyn.com terminal mockup, and a nearest 16-color code for terminals that
    // don't advertise truecolor. Descriptions and the version banner are left in
    // the terminal's default foreground on purpose, so they stay legible on both
    // dark and light backgrounds without needing to know which one it is.
    struct Role { const char* truecolor; const char* basic; };
    static const Role label   { "38;2;232;121;76",  "33" };   // terracotta: Usage/Commands headers
    static const Role command { "38;2;109;188;128", "32" };   // sage green: command and flag names
    static const Role option  { "38;2;126;167;205", "34" };   // steel blue: option placeholders and the URL

    static bool enabled()
    {
        static const bool value = terminalSupportsColor();
        return value;
    }

    // truecolor when the terminal advertises it, otherwise the 16-color palette
    static bool trueColor()
    {
        static const bool value = terminalSupportsTrueColor();
        return value;
    }

    // wraps text in the role's color, or returns it unchanged when color is off
    static String paint(const Role& role, const String& text)
    {
        if (! enabled() || text.isEmpty())
        {
            return text;
        }
        const char* code = trueColor() ? role.truecolor : role.basic;
        if (code == nullptr || *code == '\0')
        {
            return text;
        }
        return "\x1b[" + String(code) + "m" + text + reset;
    }
}

// word-wraps text into lines no wider than the given number of characters
static StringArray wrapText(const String& text, int width)
{
    StringArray words;
    words.addTokens(text, " ", "");
    words.removeEmptyStrings();

    StringArray lines;
    String current;
    for (auto&& word : words)
    {
        if (current.isEmpty())
        {
            current = word;
        }
        else if (current.length() + 1 + word.length() <= width)
        {
            current << " " << word;
        }
        else
        {
            lines.add(current);
            current = word;
        }
    }
    if (current.isNotEmpty())
    {
        lines.add(current);
    }
    return lines;
}

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
    commands_.add({"bpm",   "beats-per-minute",         BEATS_PER_MINUTE,      0, {""},                 {"Show tempo of incoming MIDI clock instead of the ticks"}});
    commands_.add({"omc",   "octave-middle-c",          OCTAVE_MIDDLE_C,       1, {"number"},           {"Set octave for middle C, defaults to 3"}});
    commands_.add({"voice", "",                         VOICE,                 0, {""},                 {"Show all Channel Voice messages"}});
    commands_.add({"note",  "",                         NOTE,                  0, {""},                 {"Show all Note messages"}});
    commands_.add({"on",    "note-on",                  NOTE_ON,              -1, {"(note)"},           {"Show Note On, optionally for note (0-127)"}});
    commands_.add({"off",   "note-off",                 NOTE_OFF,             -1, {"(note)"},           {"Show Note Off, optionally for note (0-127)"}});
    commands_.add({"pp",    "poly-pressure",            POLY_PRESSURE,        -1, {"(note)"},           {"Show Poly Pressure, optionally for note (0-127)"}});
    commands_.add({"cc",    "control-change",           CONTROL_CHANGE,       -1, {"(number)"},         {"Show Control Change, optionally for controller (0-127)"}});
    commands_.add({"cc14",  "control-change-14",        CONTROL_CHANGE_14BIT, -1, {"(number)"},         {"Show 14-bit CC, optionally for controller (0-31)"}});
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
                                                                                  {"Configure responder MPE Profile creating virtual MIDI input",
                                                                                   "and output ports with the provided name, available manager",
                                                                                   "channel (1-15 or 0 for any) and desired member channel",
                                                                                   "count (1-15, or 0 for any) (Linux/macOS)"}});
    commands_.add({"mcr",   "mpe-channel-reponse",      MPE_CHANNEL_RESPONSE,  1, {"flag"},             {"Sets MPE Profile channel response feature (0-1)"}});
    commands_.add({"mpb",   "mpe-pitch-bend",           MPE_PITCH_BEND,        1, {"flag"},             {"Sets MPE Profile pitch bend feature (0-1)"}});
    commands_.add({"mcp",   "mpe-channel-pressure",     MPE_CHANNEL_PRESSURE,  1, {"flag"},             {"Sets MPE Profile channel pressure feature (0-2)"}});
    commands_.add({"m3d",   "mpe-3rd-dimension",        MPE_3RD_DIMENSION,     1, {"flag"},             {"Sets MPE Profile 3rd dimension feature (0-2)"}});

    timestampOutput_ = false;
    noteNumbersOutput_ = false;
    octaveMiddleC_ = DEFAULT_OCTAVE_MIDDLE_C;
    useHexadecimalsByDefault_ = false;
    quiet_ = false;
    rawdump_ = false;
    bpmDisplay_ = false;
    bpmInteractive_ = false;
    lastBpmTime_ = 0.0;
    lastPrintedBpm_ = 0.0;
    avgBpm_ = 0.0;
    lastAvgTime_ = 0.0;
    clockJumpTime_ = 0.0;
    crossingSince_ = 0.0;
    bpmLineActive_ = false;
    currentCommand_ = ApplicationCommand::Dummy();
    
    mpeProfile_ = std::make_unique<MpeProfileNegotiation>();
    
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
    scriptMidiMessage_ = new ScriptMidiMessageClass(*this);
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
        scriptMidiMessage_->setDisplayState(display);
        scriptMidiMessage_->setMidiMessage(msg);
        auto result = scriptEngine_.execute(scriptCode_);
        if (result.failed())
        {
            std::cerr << result.getErrorMessage() << std::endl;
        }
    }
    
    if (!quiet_)
    {
        if (rawdump_)
        {
            dumpMessage(msg);
        }
        else
        {
            outputMessage(msg, display);
        }
    }
}

void ApplicationState::dumpMessage(const MidiMessage& msg) const
{
    std::cout.write((char *)msg.getRawData(), msg.getRawDataSize());
    std::cout.flush();
}

void ApplicationState::configure(const StringArray& params)
{
    StringArray p(params);
    parseParameters(p);
}

void ApplicationState::configureLine(const String& line)
{
    configure(parseLineAsParameters(line));
}

String ApplicationState::receive(const MidiMessage& msg)
{
    // capture what the normal receive path prints for this one message; a
    // capture is not a terminal, so the tempo readout always uses its line
    // mode here, keeping tests stable
    bool wasInteractive = bpmInteractive_;
    bpmInteractive_ = false;
    std::ostringstream captured;
    auto* previous = std::cout.rdbuf(captured.rdbuf());
    handleIncomingMidiMessage(nullptr, msg);
    std::cout.rdbuf(previous);
    bpmInteractive_ = wasInteractive;
    return captured.str();
}

void ApplicationState::outputTimestamp() const
{
    if (timestampOutput_)
    {
        Time t = Time::getCurrentTime();
        std::cout << String(t.getHours()).paddedLeft('0', 2) << ":"
        << String(t.getMinutes()).paddedLeft('0', 2) << ":"
        << String(t.getSeconds()).paddedLeft('0', 2) << "."
        << String(t.getMilliseconds()).paddedLeft('0', 3) << "   ";
    }
}

// how many MIDI clock timestamps are kept, and the tempo range reported
static constexpr int TIMESTAMP_QUEUE_SIZE = 48;
static constexpr double BPM_MIN = 20.0;
static constexpr double BPM_MAX = 360.0;

// measures the tempo of the incoming MIDI clock from the queued tick
// timestamps; returns 0 while there aren't enough clean intervals yet
double ApplicationState::measureClockBpm(double now) const
{
    // keep a queue of MIDI clock timestamps, never exceeding TIMESTAMP_QUEUE_SIZE
    clockTimes_.push_front(now);
    while (clockTimes_.size() > TIMESTAMP_QUEUE_SIZE)
    {
        clockTimes_.pop_back();
    }
    if (clockTimes_.size() < 2)
    {
        return 0.0;
    }

    // the average tick interval: the timestamps in between cancel out, so
    // only the newest and the oldest matter
    double avg = (clockTimes_.front() - clockTimes_.back()) / (double)(clockTimes_.size() - 1);

    // only count intervals close to the average, keeping the normal timing
    // differences between ticks but dropping stalls and bunched-up late ones
    double sum = 0.0;
    int kept = 0;
    for (size_t i = 0; i + 1 < clockTimes_.size(); i++)
    {
        double interval = clockTimes_[i] - clockTimes_[i + 1];
        if (std::abs(avg - interval) < avg * 0.15)
        {
            sum += interval;
            ++kept;
        }
    }
    // wait for a mostly full window: fewer intervals give a rough reading,
    // while requiring even more starves the display at high tempos, where
    // ticks are closer together and more of them get dropped above
    if (kept < TIMESTAMP_QUEUE_SIZE * 3 / 4)
    {
        return 0.0;
    }

    double bpm = int((600.0 / (sum / kept) / 24.0) + 0.5) / 10.0;
    return std::min(std::max(bpm, BPM_MIN), BPM_MAX);
}

// turns the readings into the whole BPM to display and reports whether it
// changed: single readings wobble, so a running average smooths them and the
// shown value only moves once the average clearly settles on another number
bool ApplicationState::updateShownClockBpm(double bpm, double now) const
{
    double dt = now - lastAvgTime_;
    if (lastAvgTime_ <= 0.0 || dt > 2.0)
    {
        avgBpm_ = bpm;
    }
    else
    {
        // after a tempo jump the average starts from a single reading that
        // may round to the wrong number: while it disagrees with the display
        // it catches up faster, and calms down again once they match
        bool correcting = now - clockJumpTime_ < 1.5
                          && std::abs(avgBpm_ - lastPrintedBpm_) >= 0.35;
        avgBpm_ += std::min(1.0, dt / (correcting ? 0.35 : 1.0)) * (bpm - avgBpm_);
    }
    lastAvgTime_ = now;

    // only a big change counts as a tempo jump, since the wobble at high
    // tempos can span a few BPM; small real changes reach the display
    // through the average anyway
    if (std::abs(bpm - lastPrintedBpm_) >= std::max(4.0, bpm * 0.03))
    {
        avgBpm_ = bpm;
        clockJumpTime_ = now;
    }

    if (std::abs(avgBpm_ - lastPrintedBpm_) < 0.75)
    {
        crossingSince_ = 0.0;
        return false;
    }

    // a move of several BPM shows immediately, but a step to the next
    // number has to stick around for a moment first, so a short wobble
    // can't flip the display back and forth
    double target = (double)((int)(avgBpm_ + 0.5));
    if (std::abs(target - lastPrintedBpm_) < 2.0)
    {
        if (crossingSince_ <= 0.0)
        {
            crossingSince_ = now;
            return false;
        }
        if (now - crossingSince_ < 0.6)
        {
            return false;
        }
    }
    crossingSince_ = 0.0;
    lastPrintedBpm_ = target;
    return true;
}

void ApplicationState::outputClockTempo(double now) const
{
    double bpm = measureClockBpm(now);
    if (bpm <= 0.0)
    {
        return;
    }
    bool changed = updateShownClockBpm(bpm, now);

    if (bpmInteractive_)
    {
        // an interactive terminal gets a single line that is redrawn in
        // place when the tempo changes, like an instrument display
        if ((changed || !bpmLineActive_) && now - lastBpmTime_ > 0.25)
        {
            lastBpmTime_ = now;
            std::cout << '\r';
            outputTimestamp();
            // the spaces erase leftovers of a longer previous value, the
            // backspaces put the cursor right back after the digits
            std::cout << "bpm " << (int)lastPrintedBpm_ << "  \b\b" << std::flush;
            bpmLineActive_ = true;
        }
    }
    else
    {
        // piped or redirected output gets a line right away when the tempo
        // changes, and a repeat at a steady pace in between
        if (changed || now - lastBpmTime_ > 2.0)
        {
            lastBpmTime_ = now;
            outputTimestamp();
            std::cout << "bpm " << (int)lastPrintedBpm_ << std::endl;
        }
    }
}

void ApplicationState::outputMessage(const MidiMessage& msg, DisplayState& display) const
{
    if (bpmDisplay_)
    {
        if (msg.isMidiClock())
        {
            // the tempo readout replaces the individual midi-clock lines
            outputClockTempo(msg.getTimeStamp());
            return;
        }
        if (msg.isMidiStart() || msg.isMidiContinue() || msg.isMidiStop())
        {
            // transport changes restart the measurement; the message itself
            // still prints below
            clockTimes_.clear();
            lastAvgTime_ = 0.0;
        }
        if (bpmLineActive_)
        {
            // finish the in-place tempo line before a regular line prints
            std::cout << std::endl;
            bpmLineActive_ = false;
        }
    }

    outputTimestamp();

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
                uint8 msb_cc = (uint8)msg.getControllerNumber();
                if (msb_cc >= 32)
                {
                    msb_cc -= 32;
                }
                uint8 lsb_cc = (uint8)msb_cc + 32;
                uint8 ch = (uint8)msg.getChannel() - 1;
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

String ApplicationState::output7BitAsHex(int v) const
{
    return String::toHexString(v).paddedLeft('0', 2).toUpperCase();
}

String ApplicationState::output7Bit(int v) const
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

String ApplicationState::output14BitAsHex(int v) const
{
    return String::toHexString(v).paddedLeft('0', 4).toUpperCase();
}

String ApplicationState::output14Bit(int v) const
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

String ApplicationState::outputNote(const MidiMessage& msg) const
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

String ApplicationState::outputChannel(const MidiMessage& msg) const
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
        case BEATS_PER_MINUTE:
            bpmDisplay_ = true;
            bpmInteractive_ = ansi::terminalIsInteractive();
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
            JUCEApplicationBase::getInstance()->setApplicationReturnValue(EXIT_FAILURE);
#endif
            break;
        }
        case MPE_CHANNEL_RESPONSE:
            mpeProfile_->setSupportsChannelResponse(jlimit(0, 1, asDecOrHexIntValue(cmd.opts_[0])));
            break;
        case MPE_PITCH_BEND:
            mpeProfile_->setSupportsPitchBend(jlimit(0, 1, asDecOrHexIntValue(cmd.opts_[0])));
            break;
        case MPE_CHANNEL_PRESSURE:
            mpeProfile_->setSupportsChannelPressure(jlimit(0, 2, asDecOrHexIntValue(cmd.opts_[0])));
            break;
        case MPE_3RD_DIMENSION:
            mpeProfile_->setSupportsThirdDimension(jlimit(0, 2, asDecOrHexIntValue(cmd.opts_[0])));
            break;
        case CHANNEL:
        {
            auto channel = asDecOrHexIntValue(cmd.opts_[0]);
            if (channel < 0 || channel > 16)
            {
                std::cerr << "Can't set channel to " << channel << " (it has to be between 0 and 16)" << std::endl;
                JUCEApplicationBase::getInstance()->setApplicationReturnValue(EXIT_FAILURE);
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

uint8 ApplicationState::asNoteNumber(String value) const
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

uint8 ApplicationState::asDecOrHex7BitValue(String value) const
{
    return (uint8)limit7Bit(asDecOrHexIntValue(value));
}

uint16 ApplicationState::asDecOrHex14BitValue(String value) const
{
    return (uint16)limit14Bit(asDecOrHexIntValue(value));
}

int ApplicationState::asDecOrHexIntValue(String value) const
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
    std::cout << ansi::paint(ansi::option, "https://github.com/gbevin/ReceiveMIDI") << std::endl;
}

void ApplicationState::printUsage()
{
    printVersion();
    std::cout << std::endl;
    std::cout << ansi::paint(ansi::label, "Usage:") << " " << ProjectInfo::projectName << " [ commands ] [ programfile ] [ -- ]" << std::endl << std::endl
              << ansi::paint(ansi::label, "Commands:") << std::endl << std::endl;
    // the columns follow the longest command name and option, so nothing is
    // truncated and everything stays aligned
    int longestCommand = 0;
    int longestOption = 0;
    for (auto&& cmd : commands_)
    {
        longestCommand = jmax(longestCommand, cmd.param_.length());
        for (auto&& option : cmd.optionsDescriptions_)
        {
            longestOption = jmax(longestOption, option.length());
        }
    }
    const int optionColumn = longestCommand + 3;              // where the options start
    const int descriptionColumn = optionColumn + longestOption + 1;  // where the description starts

    for (auto&& cmd : commands_)
    {
        // the options share the option column, wrapping onto extra lines only
        // when they don't all fit (reserving one space before the description)
        String joinedOptions;
        for (auto&& option : cmd.optionsDescriptions_)
        {
            if (option.isNotEmpty())
            {
                joinedOptions << (joinedOptions.isEmpty() ? "" : " ") << option;
            }
        }
        StringArray options = wrapText(joinedOptions, descriptionColumn - optionColumn - 1);

        // the description (its lines joined) starts on the first option's line
        String joinedDescription;
        for (auto&& description : cmd.commandDescriptions_)
        {
            joinedDescription << (joinedDescription.isEmpty() ? "" : " ") << description;
        }
        StringArray descriptionLines = wrapText(joinedDescription, 80 - descriptionColumn);

        for (int i = 0; i < jmax(1, options.size(), descriptionLines.size()); ++i)
        {
            // build the row piece by piece, tracking the visible column so the
            // color codes never enter the padding maths
            String out;
            int col = 0;
            auto padTo = [&](int target)
            {
                if (target > col)
                {
                    out << String::repeatedString(" ", target - col);
                    col = target;
                }
            };

            if (i == 0)
            {
                out << "  ";
                col += 2;
                out << ansi::paint(ansi::command, cmd.param_);
                col += cmd.param_.length();
            }
            padTo(optionColumn);

            if (i < options.size() && options.getReference(i).isNotEmpty())
            {
                out << ansi::paint(ansi::option, options.getReference(i));
                col += options.getReference(i).length();
            }

            if (i < descriptionLines.size())
            {
                padTo(descriptionColumn);
                out << descriptionLines.getReference(i);   // description in the default color
            }

            std::cout << out << std::endl;
        }
    }
    std::cout << std::endl;
    auto builtin = [&](const String& flag, const String& description)
    {
        const StringArray lines = wrapText(description, 80 - descriptionColumn);
        for (int i = 0; i < lines.size(); ++i)
        {
            String out;
            int col = 0;
            if (i == 0)
            {
                out << "  " << ansi::paint(ansi::command, flag);
                col += 2 + flag.length();
            }
            if (descriptionColumn > col)
            {
                out << String::repeatedString(" ", descriptionColumn - col);
            }
            std::cout << out << lines.getReference(i) << std::endl;
        }
    };
    std::cout << ansi::paint(ansi::label, "Options:") << std::endl;
    builtin("-h  or  --help", "Print Help (this message) and exit");
    builtin("--version", "Print version information and exit");
    builtin("--", "Read commands from standard input until it's closed");
    std::cout << std::endl;
    std::cout << "Alternatively, you can use the following long versions of the commands:" << std::endl;
    String line = " ";
    auto flushLine = [&]()
    {
        std::cout << ansi::paint(ansi::command, line) << std::endl;
    };
    for (auto&& cmd : commands_)
    {
        if (cmd.altParam_.isNotEmpty())
        {
            if (line.length() + cmd.altParam_.length() + 1 >= 80)
            {
                flushLine();
                line = " ";
            }
            line << " " << cmd.altParam_;
        }
    }
    flushLine();
    std::cout << std::endl;

    // tints command names and flags quoted in the trailing notes; the sentences
    // stay in the default foreground
    auto isCommandName = [this](const String& token)
    {
        for (auto&& cmd : commands_)
        {
            if (cmd.param_ == token || (cmd.altParam_.isNotEmpty() && cmd.altParam_ == token))
            {
                return true;
            }
        }
        return false;
    };
    auto note = [&isCommandName](const String& text)
    {
        if (! ansi::enabled())
        {
            return text;
        }
        String out;
        int i = 0;
        while (i < text.length())
        {
            const int open = text.indexOfChar(i, '"');
            if (open < 0) { out << text.substring(i); break; }
            const int close = text.indexOfChar(open + 1, '"');
            if (close < 0) { out << text.substring(i); break; }
            const String inner = text.substring(open + 1, close);
            const bool tint = inner.startsWith("--")
                              || (! inner.containsChar(' ') && isCommandName(inner));
            out << text.substring(i, open + 1)
                << (tint ? ansi::paint(ansi::command, inner) : inner) << "\"";
            i = close + 1;
        }
        return out;
    };
    std::cout << "By default, numbers are interpreted in the decimal system, this can be changed" << std::endl
              << note("to hexadecimal by sending the \"hex\" command. Additionally, by suffixing a") << std::endl
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
    std::cout << note("For details on how to use the \"javascript\" and \"javascript-file\" commands,") << std::endl
              << "please refer to the JAVASCRIPT.md documentation file." << std::endl;
}
