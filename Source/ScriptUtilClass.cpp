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

#include "ScriptUtilClass.h"

ScriptUtilClass::ScriptUtilClass()
{
    setMethod("command", command);
    setMethod("print", print);
    setMethod("println", println);
    setMethod("sleep", sleep);
}

String ScriptUtilClass::toString(const var::NativeFunctionArgs& a)
{
    String out;
    for (int i = 0; i < a.numArguments; ++i)
    {
        if (i != 0)
        {
            out << " ";
        }
        out << a.arguments[i].toString();
    }
    
    return out;
}

var ScriptUtilClass::command(const var::NativeFunctionArgs& a)
{
    if (a.numArguments == 0) return false;
    
    String cmd = a.arguments[0];
    ChildProcess child;
    if (child.start(cmd))
    {
        std::cout << child.readAllProcessOutput();
    }
    else
    {
        std::cerr << "Script function Util.command('" << cmd << "') couldn't be started." << std::endl;
    }
    
    return true;
}

var ScriptUtilClass::print(const var::NativeFunctionArgs& a)
{
    if (a.numArguments == 0) return false;
    
    String out = toString(a);
    std::cout << out;
    
    return true;
}

var ScriptUtilClass::println(const var::NativeFunctionArgs& a)
{
    if (a.numArguments == 0) return false;
    
    String out = toString(a);
    std::cout << out << std::endl;
    
    return true;
}

var ScriptUtilClass::sleep(const var::NativeFunctionArgs& a)
{
    if (a.numArguments == 0) return false;
    
    Thread::sleep(int(a.arguments[0]));
    
    return true;
}
