/*
 * This file is part of ReceiveMIDI.
 * Copyright (c) 2017-2024 Uwyn LLC.  https://www.uwyn.com
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

#include "ScriptOscClass.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <memory>

ScriptOscClass::ScriptOscClass()
{
    setMethod("connect", connect);
}

var ScriptOscClass::connect(const var::NativeFunctionArgs& a)
{
    if (a.numArguments < 2) return false;

    // the script engine copies the returned object's methods and doesn't keep
    // the native object itself alive, so the send closure owns the sender
    auto sender = std::make_shared<OSCSender>();
    sender->connect(a.arguments[0].toString(), int(a.arguments[1]));

    auto* obj = new DynamicObject();
    obj->setMethod("send", [sender](const var::NativeFunctionArgs& args) -> var
    {
        if (args.numArguments < 1) return false;

        // an invalid address or argument throws; that must not escape the script
        // into the MIDI thread, so it becomes a false return like other failures
        try
        {
            OSCMessage msg(args.arguments[0].toString());
            for (int i = 1; i < args.numArguments; ++i)
            {
                var arg = args.arguments[i];
                if (arg.isInt() || arg.isInt64() || arg.isBool())
                {
                    msg.addInt32(arg);
                }
                else if (arg.isDouble())
                {
                    // the script engine hands over every number as a double, so a
                    // whole number is sent as the integer the script meant
                    double value = arg;
                    if (value == std::floor(value) && value >= std::numeric_limits<int32>::min() && value <= std::numeric_limits<int32>::max())
                    {
                        msg.addInt32((int32)value);
                    }
                    else
                    {
                        msg.addFloat32((float)value);
                    }
                }
                else
                {
                    msg.addString(arg.toString());
                }
            }

            return sender->send(msg);
        }
        catch (const OSCException& e)
        {
            std::cerr << "Script function send() couldn't send an OSC message: " << e.description << std::endl;
            return false;
        }
    });

    return var(obj);
}
