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
    
    String midiInName_;
    std::unique_ptr<MidiInput> midiIn_;
    String fullMidiInName_;
    
    std::unique_ptr<MidiOutput> midiPass_;
    
    std::unique_ptr<MpeProfileNegotiation> mpeProfile_;

    std::unique_ptr<FileOutputStream> sysexOutput_;
};
