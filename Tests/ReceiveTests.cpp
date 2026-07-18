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

#include "JuceHeader.h"

#include <sstream>

#include "../Source/ApplicationState.h"
#include "../Source/ScriptUtilClass.h"

// Feeds MIDI messages through the real receive path and checks the text that is
// printed for them, and that the filter commands include or exclude the right
// messages. The column padding is squeezed to single spaces so the expected
// lines stay readable.
class ReceiveTests : public UnitTest
{
public:
    ReceiveTests() : UnitTest("Receive", "Receive") {}

    static String norm(const String& s)
    {
        String t = s.trim();
        while (t.contains("  ")) t = t.replace("  ", " ");
        return t;
    }

    void runTest() override
    {
        beginTest("Channel voice messages are formatted with note names by default");
        {
            expectEquals(norm(ApplicationState().receive(MidiMessage::noteOn(1, 60, (uint8)100))),
                         String("channel 1 note-on C3 100"));
            expectEquals(norm(ApplicationState().receive(MidiMessage::noteOff(1, 60, (uint8)64))),
                         String("channel 1 note-off C3 64"));
            expectEquals(norm(ApplicationState().receive(MidiMessage::aftertouchChange(1, 60, 40))),
                         String("channel 1 poly-pressure C3 40"));
            expectEquals(norm(ApplicationState().receive(MidiMessage::controllerEvent(2, 74, 55))),
                         String("channel 2 control-change 74 55"));
            expectEquals(norm(ApplicationState().receive(MidiMessage::programChange(1, 5))),
                         String("channel 1 program-change 5"));
            expectEquals(norm(ApplicationState().receive(MidiMessage::channelPressureChange(1, 90))),
                         String("channel 1 channel-pressure 90"));
            expectEquals(norm(ApplicationState().receive(MidiMessage::pitchWheel(1, 8192))),
                         String("channel 1 pitch-bend 8192"));
        }

        beginTest("System real-time messages are formatted");
        {
            expectEquals(norm(ApplicationState().receive(MidiMessage::midiClock())), String("midi-clock"));
            expectEquals(norm(ApplicationState().receive(MidiMessage::midiStart())), String("start"));
            expectEquals(norm(ApplicationState().receive(MidiMessage::midiStop())),  String("stop"));
            expectEquals(norm(ApplicationState().receive(MidiMessage::midiContinue())), String("continue"));
        }

        beginTest("The nn setting prints notes as numbers");
        {
            ApplicationState s;
            s.configureLine("nn");
            expectEquals(norm(s.receive(MidiMessage::noteOn(1, 60, (uint8)100))),
                         String("channel 1 note-on 60 100"));
        }

        beginTest("The omc setting shifts the note names");
        {
            ApplicationState s;
            s.configureLine("omc 4");   // middle C is now C4, so note 60 is C4
            expectEquals(norm(s.receive(MidiMessage::noteOn(1, 60, (uint8)100))),
                         String("channel 1 note-on C4 100"));
        }

        beginTest("A message-type filter shows only matching messages");
        {
            ApplicationState s;
            s.configureLine("on");   // only note-ons pass
            expect(s.receive(MidiMessage::noteOn(1, 60, (uint8)100)).isNotEmpty());
            expect(s.receive(MidiMessage::controllerEvent(1, 74, 55)).isEmpty());
            expect(s.receive(MidiMessage::noteOff(1, 60, (uint8)0)).isEmpty());
        }

        beginTest("A channel filter narrows a type filter to that channel");
        {
            // the channel is set before the type filter so it applies to it
            ApplicationState s;
            s.configureLine("ch 1 voice");
            expect(s.receive(MidiMessage::noteOn(1, 60, (uint8)100)).isNotEmpty());
            expect(s.receive(MidiMessage::noteOn(2, 60, (uint8)100)).isEmpty());
        }

        beginTest("An out-of-range channel filter is rejected, not silently applied");
        {
            // channel 20 is invalid, so the ch filter is dropped and only the type
            // filter remains, letting every channel through instead of matching none
            ApplicationState s;
            s.configureLine("ch 20 voice");
            expect(s.receive(MidiMessage::noteOn(3, 60, (uint8)100)).isNotEmpty());
        }

        beginTest("Without a filter every message passes");
        {
            // no configure: an empty filter list shows everything
            expect(ApplicationState().receive(MidiMessage::controllerEvent(1, 7, 100)).isNotEmpty());
        }

        beginTest("The bpm setting shows the clock tempo instead of the ticks");
        {
            ApplicationState s;
            s.configureLine("bpm");

            auto tick = [&](double time)
            {
                MidiMessage clk = MidiMessage::midiClock();
                clk.setTimeStamp(time);
                return s.receive(clk);
            };

            // a steady clock at 20ms per tick is 125 BPM (24 ticks per beat);
            // one line prints as soon as enough ticks arrived
            String out;
            double time = 3600.0;   // real timestamps are seconds since boot
            out << tick(time);
            for (int t = 0; t < 99; ++t)
            {
                time += 0.020;
                out << tick(time);
            }
            expect(!out.contains("midi-clock"));
            expectEquals(norm(out), String("bpm 125"));

            // a burst of late-delivered ticks is filtered out: the repeat
            // line that falls into this stretch still reads exactly 125
            out.clear();
            for (int t = 0; t < 5; ++t)
            {
                time += 0.001;
                out << tick(time);
            }
            for (int t = 0; t < 48; ++t)
            {
                time += 0.020;
                out << tick(time);
            }
            expectEquals(norm(out), String("bpm 125"));

            // slowing down to 30ms per tick is a real tempo jump: it prints
            // right away once the new tempo takes over, and then repeats
            out.clear();
            for (int t = 0; t < 240; ++t)
            {
                time += 0.030;
                out << tick(time);
            }
            StringArray lines = StringArray::fromLines(out.trim());
            expect(lines.size() >= 2);
            for (const String& line : lines)
            {
                expectEquals(norm(line), String("bpm 83"));
            }

            // a transport message restarts the measurement and still prints
            expectEquals(norm(s.receive(MidiMessage::midiStart())), String("start"));
        }

#if ! JUCE_WINDOWS
        beginTest("Util.command passes quoted arguments without the quotes");
        {
            JavascriptEngine engine;
            engine.registerNativeObject("Util", new ScriptUtilClass());

            // the child's output goes through std::cout, so capture it there
            std::ostringstream captured;
            auto* previous = std::cout.rdbuf(captured.rdbuf());
            auto r1 = engine.execute("Util.command('/bin/echo one \"two words\"');");
            auto r2 = engine.execute("Util.command('/bin/echo', 'a b', 'c');");
            std::cout.rdbuf(previous);

            expect(r1.wasOk(), r1.getErrorMessage());
            expect(r2.wasOk(), r2.getErrorMessage());
            expectEquals(String(captured.str()), String("one two words\na b c\n"));
        }
#endif
    }
};

static ReceiveTests receiveTests;
