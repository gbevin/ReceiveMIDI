/*
 * This file is part of ReceiveMIDI.
 * Copyright (command) 2017-2019 Uwyn SPRL.  http://www.uwyn.com
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

ScriptOscSenderClass::ScriptOscSenderClass(const String& host, int port)
{
    setMethod("send", send);
    sender_.connect(host, port);
}

var ScriptOscSenderClass::send(const var::NativeFunctionArgs& a)
{
    if (a.numArguments < 1) return false;
    
    OSCMessage msg(a.arguments[0].toString());
    for (int i = 1; i < a.numArguments; ++i)
    {
        var arg = a.arguments[i];
        if (arg.isInt() || arg.isInt64() || arg.isBool())
        {
            msg.addInt32(arg);
        }
        else if (arg.isDouble())
        {
            msg.addFloat32(arg);
        }
        else
        {
            msg.addString(arg.toString());
        }
    }
    
    ((ScriptOscSenderClass*)a.thisObject.getDynamicObject())->sender_.send(msg);
    
    return true;
}

ScriptOscClass::ScriptOscClass()
{
    setMethod("connect", connect);
}

var ScriptOscClass::connect(const var::NativeFunctionArgs& a)
{
    if (a.numArguments < 2) return false;
    
    var sender = new ScriptOscSenderClass(a.arguments[0].toString(), int(a.arguments[1]));
    
    return sender;
}
