# ReceiveMIDI JavaScript support

ReceiveMIDI supports running JavaScript code for each received MIDI message.
By using the individual show MIDI message commands, the messages can be filtered out beforehand.

Note that this not fully standards-compliant, and is not as fast as the fancy JIT-compiled engines that you get in browsers, but this is an extremely compact, low-overhead JavaScript interpreter that comes with JUCE.

JavaScript code can be provided directly on the command line by using `js` or `javascript`, but the code can also be read from a file by using the `jsf` and `javascript-file` commands.

Only one script can be active, the last code that is provided will be the code that is executed.

The engine that is running the code is stateful and processes all the MIDI messages with the same code. This can be used to make more complex decisions about what to do by looking at multiple MIDI messages and keeping track of the state in global variables.

For instance, using the file `osc.js`:

```
receivemidi dev linnstrument jsf /path/to/osc.js
```

In `osc.js` an OSC connection is established and re-used for each message, and a count of note on messages is maintained:

```javascript
if (osc === undefined)
{
    osc = OSC.connect('127.0.0.1', 12800);
}

if (MIDI.isNoteOn())
{ 
    if (count === undefined)
    {
        count = 0;
    }
    count++;

    osc.send('/note-on', count, MIDI.noteNumber(), MIDI.velocity());
}
else if (MIDI.isNoteOff())
{ 
    osc.send('/note-off', MIDI.noteNumber(), MIDI.velocity());
}
```

Additionally to all the usual JavaScript primitives and commands, ReceiveMIDI provides the following JavaScript functions:

## General Utilities

```javascript
Util.command('/full/path/to/executable arguments');
Util.println('some text');
Util.sleep(<milliseconds>);
```

## Sending OSC messages

```javascript
osc_sender = OSC.connect("hostname", port);
osc_sender.send("/osc/path", arg1, arg2, ...);
```

## Checking and retrieving data from the current MIDI message

```javascript
MIDI.getRawData();        // array
MIDI.getRawDataSize();
MIDI.rawData();           // array
MIDI.rawDataSize();

MIDI.getDescription();
MIDI.description();

MIDI.getTimeStamp();
MIDI.getChannel();
MIDI.timeStamp();
MIDI.channel();

MIDI.isSysEx();
MIDI.getSysExData();      // array
MIDI.getSysExDataSize();
MIDI.sysExData();         // array
MIDI.sysExDataSize();

MIDI.isNoteOn();
MIDI.isNoteOff();
MIDI.isNoteOnOrOff();
MIDI.getNoteNumber();
MIDI.getVelocity();
MIDI.getFloatVelocity();
MIDI.noteNumber();
MIDI.velocity();
MIDI.floatVelocity();

MIDI.isSustainPedalOn();
MIDI.isSustainPedalOff();
MIDI.isSostenutoPedalOn();
MIDI.isSostenutoPedalOff();
MIDI.isSoftPedalOn();
MIDI.isSoftPedalOff();

MIDI.isProgramChange();
MIDI.getProgramChangeNumber();
MIDI.programChange();

MIDI.isPitchWheel();
MIDI.isPitchBend();
MIDI.getPitchWheelValue();
MIDI.getPitchBendValue();
MIDI.pitchWheel();
MIDI.pitchBend();

MIDI.isAftertouch();
MIDI.isPolyPressure();
MIDI.getAfterTouchValue();
MIDI.getPolyPressureValue();
MIDI.afterTouch();
MIDI.polyPressure();

MIDI.isChannelPressure();
MIDI.getChannelPressureValue();
MIDI.channelPressure();

MIDI.isController();
MIDI.getControllerNumber();
MIDI.getControllerValue();
MIDI.controllerNumber();
MIDI.controllerValue();

MIDI.isAllNotesOff();
MIDI.isAllSoundOff();
MIDI.isResetAllControllers();

MIDI.isActiveSense();
MIDI.isMidiStart();
MIDI.isMidiContinue();
MIDI.isMidiStop();
MIDI.isMidiClock();
MIDI.isSongPositionPointer();
MIDI.getSongPositionPointerMidiBeat();
MIDI.songPositionPointerMidiBeat();
```