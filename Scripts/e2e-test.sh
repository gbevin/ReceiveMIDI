#!/usr/bin/env bash
#
# End-to-end tests that drive receivemidi through real MIDI ports, with sendmidi
# as the source, so port enumeration, filtering, display settings and the MIDI
# backend are exercised on the real binaries. On macOS and Linux receivemidi
# creates a virtual port per case; on Windows E2E_PORT names an existing
# loopback port (loopMIDI) that both tools open.
#
# Usage: e2e-test.sh <path-to-receivemidi> <path-to-sendmidi>
#
# Every case brackets its traffic with CC 119 marker messages: the start marker
# is repeated until it shows up in the receiver's output, which proves both ends
# are open, and the end marker tells when everything before it has arrived.

set -u
if [ -n "${E2E_TRACE:-}" ]; then
    set -x
fi

RECEIVEMIDI="$1"
SENDMIDI="$2"
PORT="${E2E_PORT:-}"
WORK="$(mktemp -d)"
MARK_START='control-change +(119 +1|77 +01)$'
MARK_END='control-change +(119 +2|77 +02)$'
failures=0
receiver_pid=""
port=""
received=""
marker_channel=1

virtual_ports() { [ -z "$PORT" ]; }

pass() { echo "ok   $1"; }

fail() {
    echo "FAIL $1"
    shift
    printf '     %s\n' "$@"
    failures=$((failures+1))
}

# compares two multi-line strings and reports the difference
check() {
    local name="$1" expected="$2" actual="$3"
    if [ "$expected" = "$actual" ]; then
        pass "$name"
    else
        fail "$name"
        echo "--- expected -------"; printf '%s\n' "$expected"
        echo "--- actual ---------"; printf '%s\n' "$actual"
        echo "--------------------"
    fi
}

# a fresh port name per case, so a port lingering from an earlier case can't
# satisfy a later one
new_port() {
    if virtual_ports; then
        port="E2E receivemidi $$ $RANDOM"
    else
        port="$PORT"
    fi
}

# starts a tool in the background with its output in the given file; the MIDI
# backend may refuse a virtual port created right after one vanished, so a
# refused start is tried again
start_background() {
    local out="$1"
    shift
    local attempt
    for attempt in 1 2 3; do
        "$@" > "$out" 2>&1 &
        started_pid=$!
        sleep 1
        if ! grep -q "Couldn't create virtual MIDI" "$out"; then
            return 0
        fi
        kill "$started_pid" 2>/dev/null
        wait "$started_pid" 2>/dev/null
    done
    return 1
}

stop_receiver() {
    if [ -n "$receiver_pid" ]; then
        kill "$receiver_pid" 2>/dev/null
        wait "$receiver_pid" 2>/dev/null
        receiver_pid=""
        # the MIDI backend may refuse a port created right after one vanished
        sleep 0.5
    fi
}

# waits for the start marker to come through the receiver whose output is in
# the given file, sending it every quarter second
wait_for_receiver() {
    local out="$1"
    local i
    for i in $(seq 1 40); do
        sleep 0.25
        "$SENDMIDI" dev "$port" ch "$marker_channel" cc 119 1 > /dev/null 2>&1
        if grep -qE "$MARK_START" "$out"; then
            return 0
        fi
    done
    return 1
}

# starts receivemidi on the case's port with the given arguments and waits for
# it to be open; filters passed in must let CC 119 pass on $marker_channel
start_receiver() {
    local out="$1"
    shift
    local attempt
    for attempt in 1 2 3; do
        new_port
        if virtual_ports; then
            "$RECEIVEMIDI" virt "$port" "$@" > "$out" 2>&1 &
        else
            "$RECEIVEMIDI" dev "$port" "$@" > "$out" 2>&1 &
        fi
        receiver_pid=$!
        if wait_for_receiver "$out"; then
            return 0
        fi
        if ! grep -q "Couldn't create virtual MIDI" "$out"; then
            break
        fi
        stop_receiver
    done
    echo "     receiver output:"
    sed 's/^/     | /' "$out"
    return 1
}

# sends the end marker, waits for it, stops the receiver and leaves the lines
# between the markers in $received
finish_receiver() {
    local out="$1"
    local i
    for i in $(seq 1 40); do
        "$SENDMIDI" dev "$port" ch "$marker_channel" cc 119 2 > /dev/null 2>&1
        if grep -qE "$MARK_END" "$out"; then
            break
        fi
        sleep 0.25
    done
    stop_receiver
    received="$(tr -d '\r' < "$out" | awk -v s="$MARK_START" -v e="$MARK_END" \
        '$0 ~ s { buf = ""; next } $0 ~ e { printf "%s", buf; exit } { buf = buf $0 "\n" }')"
}

send() {
    "$SENDMIDI" dev "$port" "$@"
}

# runs one receiver configuration against a fixed set of messages, covering
# channels 1 and 2 and every kind of message the filters distinguish, and
# compares what it prints
run_case() {
    local name="$1" expected="$2"
    shift 2
    if start_receiver "$WORK/case.txt" "$@"; then
        send on 60 100 cc 74 64 cc14 1 8192 nrpn 300 1000 rpn 0 2 pb 100 \
             ch 2 on 61 50 off 61 0 pp 61 9 cc 1 2 pc 3 cp 4 \
             mc tc 1 5 hex syx 7E 7F 09 01
        finish_receiver "$WORK/case.txt"
        check "$name" "$expected" "$received"
    else
        fail "$name" "the receiver never saw the start marker" "$(cat "$WORK/case.txt")"
        stop_receiver
    fi
}

trap 'stop_receiver; rm -rf "$WORK"' EXIT

# --- the port is listed and every message type prints in the text format ----
if start_receiver "$WORK/list.txt"; then
    if "$SENDMIDI" list | grep -qF "$port"; then
        pass "the receiver's port is listed as an output for the other side"
    else
        fail "the receiver's port is listed as an output for the other side" "$("$SENDMIDI" list)"
    fi
    finish_receiver "$WORK/list.txt"
else
    fail "the receiver's port is listed as an output for the other side" "the receiver never saw the start marker" "$(cat "$WORK/list.txt")"
    stop_receiver
fi

run_case "every message type prints in the text format" 'channel  1   note-on           C3 100
channel  1   control-change    74    64
channel  1   control-change     1    64
channel  1   control-change    33     0
channel  1   control-change    99     2
channel  1   control-change    98    44
channel  1   control-change     6     7
channel  1   control-change    38   104
channel  1   control-change   101   127
channel  1   control-change   100   127
channel  1   control-change   101     0
channel  1   control-change   100     0
channel  1   control-change     6     0
channel  1   control-change    38     2
channel  1   control-change   101   127
channel  1   control-change   100   127
channel  1   pitch-bend           100
channel  2   note-on          C#3  50
channel  2   note-off         C#3   0
channel  2   poly-pressure    C#3   9
channel  2   control-change     1     2
channel  2   program-change         3
channel  2   channel-pressure       4
midi-clock
time-code  1 5
system-exclusive hex 7E 7F 09 01 dec'

# --- filters narrow the output, a channel filter scopes the ones after it ----
run_case "a note filter shows only notes" 'channel  1   note-on           C3 100
channel  2   note-on          C#3  50
channel  2   note-off         C#3   0' note cc 119

run_case "a control change filter with a number shows only that controller" \
    'channel  1   control-change    74    64' cc 74 cc 119

marker_channel=2
run_case "a channel filter scopes the filters that follow it" 'channel  2   note-on          C#3  50
channel  2   note-off         C#3   0
channel  2   control-change     1     2' ch 2 note cc
marker_channel=1

run_case "a 14-bit filter pairs the MSB and LSB controllers" 'channel  1   cc14               1  8192
channel  1   cc14               1  8192
channel  1   cc14               6   896
channel  1   cc14               6  1000
channel  1   cc14               6     0
channel  1   cc14               6     2
channel  2   cc14               1   256' cc14 cc 119

run_case "NRPN and RPN filters decode the parameter sequences" 'channel  1   nrpn             300  1000
channel  1   rpn                0     2' nrpnf rpnf cc 119

# --- display settings: note numbers, octave, hex and timestamps -------------
run_case "nn prints notes as numbers and omc shifts the octave" 'channel  1   note-on           60 100
channel  2   note-on           61  50
channel  2   note-off          61   0' nn omc 4 note cc 119

run_case "hex prints every number in hexadecimal" 'channel 01   note-on           C3  64
channel 02   note-on          C#3  32
channel 02   note-off         C#3  00' hex note cc 77

if start_receiver "$WORK/ts.txt" ts; then
    send on 60 100
    finish_receiver "$WORK/ts.txt"
    if printf '%s\n' "$received" | grep -qE '^[0-9]{2}:[0-9]{2}:[0-9]{2}\.[0-9]{3} +channel  1   note-on           C3 100$'; then
        pass "ts prefixes each message with a timestamp"
    else
        fail "ts prefixes each message with a timestamp" "$received"
    fi
else
    fail "ts prefixes each message with a timestamp" "the receiver never saw the start marker"
    stop_receiver
fi

# --- a script runs for each message, given inline and from a file ----------
if start_receiver "$WORK/js.txt" js "if (MIDI.isNoteOn()) Util.println('script saw note ' + MIDI.noteNumber());"; then
    send on 60 100
    finish_receiver "$WORK/js.txt"
    if printf '%s\n' "$received" | grep -q '^script saw note 60$'; then
        pass "js runs the script for each message"
    else
        fail "js runs the script for each message" "$received"
    fi
else
    fail "js runs the script for each message" "the receiver never saw the start marker"
    stop_receiver
fi

printf "if (MIDI.isNoteOn()) Util.println('file script saw note ' + MIDI.noteNumber());\n" > "$WORK/script.js"
if start_receiver "$WORK/jsf.txt" jsf "$WORK/script.js"; then
    send on 62 100
    finish_receiver "$WORK/jsf.txt"
    if printf '%s\n' "$received" | grep -q '^file script saw note 62$'; then
        pass "jsf runs the script from a file"
    else
        fail "jsf runs the script from a file" "$received"
    fi
else
    fail "jsf runs the script from a file" "the receiver never saw the start marker"
    stop_receiver
fi

# --- SysEx capture to a file and the raw dump -------------------------------
python3 - "$WORK/big.syx" <<'PY' 2>/dev/null || printf '\xF0\x7D\x01\x02\x03\x7F\xF7' > "$WORK/big.syx"
import sys
open(sys.argv[1], "wb").write(bytes([0xF0, 0x7D] + [i % 128 for i in range(200)] + [0xF7]))
PY
if start_receiver "$WORK/syf.txt" syf "$WORK/captured.syx" cc 119; then
    send syf "$WORK/big.syx" > /dev/null
    finish_receiver "$WORK/syf.txt"
    check "syf reports the SysEx and stores it in the file" \
        "system-exclusive-file $(wc -c < "$WORK/big.syx" | tr -d ' ') bytes" "$received"
    if cmp -s "$WORK/big.syx" "$WORK/captured.syx"; then
        pass "the captured SysEx file is byte-identical"
    else
        fail "the captured SysEx file is byte-identical" "$(od -An -tx1 "$WORK/captured.syx" | head -3)"
    fi
else
    fail "syf reports the SysEx and stores it in the file" "the receiver never saw the start marker" "$(cat "$WORK/syf.txt")"
    stop_receiver
fi

new_port
if virtual_ports; then
    start_background "$WORK/dump.bin" "$RECEIVEMIDI" virt "$port" dump
else
    start_background "$WORK/dump.bin" "$RECEIVEMIDI" dev "$port" dump
fi
receiver_pid=$started_pid
sleep 1
send on 60 100 off 60 0
sleep 1
stop_receiver
check "dump writes the raw bytes" "90 3c 64 80 3c 00" \
    "$(od -An -tx1 -v "$WORK/dump.bin" | tr -s ' \n' ' ' | sed 's/^ //; s/ $//')"

# --- pass-through and configuration from standard input --------------------
if virtual_ports; then
    sink_name="E2E receivemidi sink $$ $RANDOM"
    start_background "$WORK/sink.txt" "$RECEIVEMIDI" virt "$sink_name"
    sink=$started_pid
    # pass opens its output right away, so the sink has to be listed first
    for i in $(seq 1 20); do
        sleep 0.5
        if "$SENDMIDI" list | grep -qF "$sink_name"; then
            break
        fi
    done
    if start_receiver "$WORK/pass.txt" pass "$sink_name"; then
        send on 65 100
        finish_receiver "$WORK/pass.txt"
        sleep 0.5
        kill $sink 2>/dev/null
        wait $sink 2>/dev/null
        check "pass forwards every received message to the output port" \
            "$(printf 'channel  1   control-change   119     1\nchannel  1   note-on           F3 100\nchannel  1   control-change   119     2')" \
            "$(tr -d '\r' < "$WORK/sink.txt" | awk '/119 +1$/ { seen = 1; buf = "" } seen { buf = buf $0 "\n" } /119 +2$/ && seen { printf "%s", buf; exit }')"
    else
        fail "pass forwards every received message to the output port" "the receiver never saw the start marker"
        stop_receiver
        kill $sink 2>/dev/null
        wait $sink 2>/dev/null
    fi
fi

new_port
for attempt in 1 2 3; do
    if virtual_ports; then
        printf 'virt "%s"\nnn\nnote\ncc 119\n' "$port" | "$RECEIVEMIDI" -- > "$WORK/stdin.txt" 2>&1 &
    else
        printf 'dev "%s"\nnn\nnote\ncc 119\n' "$port" | "$RECEIVEMIDI" -- > "$WORK/stdin.txt" 2>&1 &
    fi
    receiver_pid=$!
    sleep 1
    if ! grep -q "Couldn't create virtual MIDI" "$WORK/stdin.txt"; then
        break
    fi
    stop_receiver
done
if wait_for_receiver "$WORK/stdin.txt"; then
    send on 60 100 cc 1 1
    finish_receiver "$WORK/stdin.txt"
    check "commands read from standard input configure the receiver" 'channel  1   note-on           60 100' "$received"
else
    fail "commands read from standard input configure the receiver" "the receiver never saw the start marker" "$(cat "$WORK/stdin.txt")"
    stop_receiver
fi

# --- the MPE Profile responder, including the optional feature details -----
if virtual_ports; then
    name="E2E receivemidi mpe $$ $RANDOM"
    start_background "$WORK/mpe-responder.txt" "$RECEIVEMIDI" mpp "$name" 2 3 mcr 1 mpb 1 mcp 2 m3d 1
    responder=$started_pid
    sleep 2
    "$SENDMIDI" dev "$name" mpp "$name" 2 3 > "$WORK/mpe-initiator.txt" 2>&1
    sleep 1
    kill $responder 2>/dev/null
    wait $responder 2>/dev/null
    EXPECTED='Responder MUID waiting for MPE Profile negotiation on channel 2
MUID : MPE Profile enabled with manager channel 2 and 3 member channels
MUID : MPE Profile details inquired for optional features'
    check "the MPE Profile responder enables the profile and answers the inquiry" \
        "$EXPECTED" "$(tr -d '\r' < "$WORK/mpe-responder.txt" | sed -E 's/MUID 0x[0-9a-f]+/MUID/')"
    EXPECTED='MUID   channel response : supported
MUID   pitch bend       : supported
MUID   channel pressure : alternate bipolar controller
MUID   3rd dimension    : standard controller'
    check "the initiator reads back the responder's optional features" \
        "$EXPECTED" "$(tr -d '\r' < "$WORK/mpe-initiator.txt" | sed -E 's/MUID 0x[0-9a-f]+/MUID/' | grep '^MUID   ')"
fi

echo
if [ "$failures" -eq 0 ]; then
    echo "all end-to-end tests passed"
else
    echo "$failures end-to-end test(s) failed"
    exit 1
fi
