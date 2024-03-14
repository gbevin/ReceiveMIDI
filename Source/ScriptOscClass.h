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

class ScriptOscSenderClass : public DynamicObject
{
public:
    ScriptOscSenderClass(const String& host, int port);
    
    static var send(const var::NativeFunctionArgs&);
    
private:
    OSCSender sender_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScriptOscSenderClass)
};

class ScriptOscClass : public DynamicObject
{
public:
    ScriptOscClass();
    
    static var connect(const var::NativeFunctionArgs&);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScriptOscClass)
};
