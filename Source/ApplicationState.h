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

#include <deque>

#include "JuceHeader.h"

#include "ApplicationCommand.h"
#include "MpeProfileNegotiation.h"
#include "ScriptMidiMessageClass.h"

class ApplicationState : public MidiInputCallback, public Timer
{
public:
    ApplicationState();
    void initialise(JUCEApplicationBase& app);
    void shutdown();
    
    static std::unique_ptr<MidiOutput> openOutputDevice(const String& name);

    uint8 asNoteNumber(String value) const;
    uint8 asDecOrHex7BitValue(String value) const;
    uint16 asDecOrHex14BitValue(String value) const;
    
    void outputMessage(const MidiMessage& msg, DisplayState& display) const;

    // Test seam: configure the receiver from command-line arguments (filters and
    // settings only - not device, virtual or file tokens, which open ports or
    // read files), then feed one MIDI message and return the text it prints
    // (empty when the message is filtered out). No MIDI port is opened.
    void configure(const StringArray& params);
    void configureLine(const String& line);
    String receive(const MidiMessage& msg);

    MidiRPNDetector rpnDetector_;
    MidiRPNMessage rpnMsg_;
    
    int lastCC_[16][128];

private:
    bool isMidiInDeviceAvailable(const String& name);
    void timerCallback() override;
    
    ApplicationCommand* findApplicationCommand(const String& param);
    StringArray parseLineAsParameters(const String& line);
    void executeCurrentCommand();
    void handleVarArgCommand();
    void openInputDevice(const String& name);
    void parseParameters(StringArray& parameters);
    void parseFile(File file);
    void handleIncomingMidiMessage(MidiInput*, const MidiMessage& msg) override;
    void dumpMessage(const MidiMessage& msg) const;
    void outputTimestamp() const;
    double measureClockBpm(double now) const;
    bool updateShownClockBpm(double bpm, double now) const;
    void outputClockTempo(double now) const;
    String output7BitAsHex(int v) const;
    String output7Bit(int v) const;
    String output14BitAsHex(int v) const;
    String output14Bit(int v) const;
    String outputNote(const MidiMessage& msg) const;
    String outputChannel(const MidiMessage& msg) const;
    bool tryToConnectMidiInput();
    void executeCommand(ApplicationCommand& cmd);
    
    int asDecOrHexIntValue(String value) const;
    static uint8 limit7Bit(int value);
    static uint16 limit14Bit(int value);
    void printVersion();
    void printUsage();

    Array<ApplicationCommand> commands_;
    Array<ApplicationCommand> filterCommands_;
    ApplicationCommand currentCommand_;
    JavascriptEngine scriptEngine_;
    String scriptCode_;
    ScriptMidiMessageClass* scriptMidiMessage_;
    
    bool timestampOutput_;
    bool noteNumbersOutput_;
    int octaveMiddleC_;
    bool useHexadecimalsByDefault_;
    bool quiet_;
    bool rawdump_;

    // tempo readout for the bpm option: a single line updated in place on an
    // interactive terminal, separate lines otherwise; mutable because the
    // const output path keeps the measurement up to date
    bool bpmDisplay_;
    bool bpmInteractive_;
    mutable std::deque<double> clockTimes_;
    mutable double lastBpmTime_;
    mutable double lastPrintedBpm_;
    mutable double avgBpm_;
    mutable double lastAvgTime_;
    mutable double clockJumpTime_;
    mutable double crossingSince_;
    mutable bool bpmLineActive_;
    
    String midiInName_;
    std::unique_ptr<MidiInput> midiIn_;
    String fullMidiInName_;
    
    std::unique_ptr<MidiOutput> midiPass_;
    
    std::unique_ptr<MpeProfileNegotiation> mpeProfile_;

    std::unique_ptr<FileOutputStream> sysexOutput_;
};
