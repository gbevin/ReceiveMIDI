# ReceiveMIDI

ReceiveMIDI is a multi-platform command-line tool makes it very easy to quickly receive and monitor MIDI messages from MIDI devices on your computer.

All the heavy lifting is done by the wonderful JUCE library.

The project website is https://github.com/gbevin/ReceiveMIDI

## Purpose
This tool is mainly intended for quickly monitoring the messages that are sent to your computer from a particular MIDI device. By providing filter commands, it's possible to only focus on particular MIDI messages.

Here's a tutorial video about both SendMIDI and ReceiveMIDI, including some tips and tricks of how to use the command-line on macOS:

<a href="https://www.youtube.com/watch?v=_o1kg0IbetY" target="_blank"><img src="https://i.ytimg.com/vi/_o1kg0IbetY/maxresdefault.jpg" alt="Tutorial Video" width="640" height="360" border="2" /></a>

## Download

You can download pre-built binaries from the release section:
https://github.com/gbevin/ReceiveMIDI/releases

Since ReceiveMIDI is free and open-source, you can also easily build it yourself. Just take a look into the Builds directory when you download the sources.

If you're using the macOS Homebrew package manager, you can install ReceiveMIDI with:
```
brew install gbevin/tools/receivemidi
```

## Usage
To use it, simply type "receivemidi" or "receivemidi.exe" on the command line and follow it with a series of commands that you want to execute. These commands have purposefully been chosen to be concise and easy to remember, so that it's extremely fast and intuitive to quickly receive or monitor MIDI messages.

These are all the supported commands:
```
  dev   name           Set the name of the MIDI input port (REQUIRED)
  list                 Lists the MIDI input ports
  file  path           Loads commands from the specified program file
  dec                  Interpret the next numbers as decimals by default
  hex                  Interpret the next numbers as hexadecimals by default
  ch    number         Set MIDI channel for the commands (0-16), defaults to 0
  ts                   Output a timestamp for each received MIDI message
  nn                   Output notes as numbers instead of names
  omc   number         Set octave for middle C, defaults to 5
  voice                Show all Channel Voice messages
  note                 Show all Note messages
  on    (note)         Show Note On, optionally for note (0-127)
  off   (note)         Show Note Off, optionally for note (0-127)
  pp    (note)         Show Poly Pressure, optionally for note (0-127)
  cc    (number)       Show Control Change, optionally for controller (0-127)
  pc    (number)       Show Program Change, optionally for program (0-127)
  cp                   Show Channel Pressure
  pb                   Show Pitch Bend
  sr                   Show all System Real-Time messages
  clock                Show Timing Clock
  start                Show Start
  stop                 Show Stop
  cont                 Show Continue
  as                   Show Active Sensing
  rst                  Show Reset
  sc                   Show all System Common messages
  syx                  Show System Exclusive
  tc                   Show MIDI Time Code Quarter Frame
  spp                  Show Song Position Pointer
  ss                   Show Song Select
  tun                  Show Tune Request
  --                   Read commands from standard input until it's closed
```

Alternatively, you can use the following long versions of the commands:
```
  device decimal hexadecimal channel timestamp note-numbers octave-middle-c
  note-on note-off poly-pressure control-change program-change channel-pressure
  pitch-bend system-realtime continue active-sensing reset system-common
  system-exclusive time-code song-position song-select tune-request
```

By default, numbers are interpreted in the decimal system, this can be changed to hexadecimal by sending the "hex" command.
Additionally, by suffixing a number with "M" or "H", it will be interpreted as a decimal or hexadecimal respectively.

The MIDI device name doesn't have to be an exact match.
If ReceiveMIDI can't find the exact name that was specified, it will pick the first MIDI output port that contains the provided text, irrespective of case.

Where notes can be provided as arguments, they can also be written as note names, by default from C0 to G10 which corresponds to note numbers 0 to 127. By setting the octave for middle C, the note name range can be changed. Sharps can be added by using the '#' symbol after the note letter, and flats by using the letter 'b'.

## Examples
  
Here are a few examples to get you started:

List all the available MIDI output ports on your system

```
receivemidi list
```

Receive all the MIDI messages coming from LinnStrument:

```
receivemidi dev "LinnStrument MIDI"
```

Receive only notes coming from LinnStrument:

```
receivemidi dev "LinnStrument MIDI" note
```

Receive all voice messages coming from LinnStrument, and also MIDI clock start and stop, all displayed with a timestamp:

```
receivemidi dev "LinnStrument MIDI" ts voice start stop
```

Receive all messages from LinnStrument and pipe them to the SendMIDI tool in order to forward them to Bidule:

```
receivemidi dev linnstrument | sendmidi dev "Bidule  1" --
```

## Text File Format

The text file that can be read through the "file" command can contain a list of commands and options, just like when you would have written them manually on the console (without the "sendmidi" executable). You can insert new lines instead of spaces and any line that starts with a hash (#) character is a comment.

For instance, this is a text file for one of the examples above:
```
dev "LinnStrument MIDI"
# timestamp the output
ts
# show all voice messages
voice
# show MIDI clock start and stop
start stop
```

## SendMIDI compatibility

The output of the ReceiveMIDI tool is compatible with the SendMIDI tool, allowing you to store MIDI message sequences and play them back later. By using Unix-style pipes on the command-line, it's even possible to chain the receivemidi and sendmidi commands in order to forward MIDI messages.

If you changed the octave for middle C or are outputting hexadecimal numbers instead of the default decimal numbers, make sure that you set up SendMIDI with the same parameters.

SendMIDI can be downloaded from https://github.com/gbevin/SendMIDI
