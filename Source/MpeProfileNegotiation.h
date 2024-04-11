/*
 * This file is part of ReceiveMIDI.
 * Copyright (command) 2017-2024 Uwyn LLC.  https://www.uwyn.com
 *
 * SendMIDI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SendMIDI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "JuceHeader.h"

class ApplicationState;

class MpeProfileNegotiation : ci::DeviceListener, ci::ProfileDelegate, ci::DeviceMessageHandler, MidiInputCallback
{
public:
    MpeProfileNegotiation();
    void processMessage(ump::BytesOnGroup) override;
    
    void setProfileMidiName(const String& name);
    void setManager(int manager);
    void setMembers(int members);
    void setSupportsChannelResponse(int flag);
    void setSupportsPitchBend(int flag);
    void setSupportsChannelPressure(int flag);
    void setSupportsThirdDimension(int flag);

private:
    static std::string muidToString(ci::MUID muid);

    void addMpeProfile(int manager);

    virtual void handleIncomingMidiMessage(MidiInput* source,
                                           const MidiMessage& message) override;

    virtual void profileEnablementRequested(ci::MUID muid,
                                            ci::ProfileAtAddress profileAtAddress,
                                            int numChannels,
                                            bool enabled) override;

    void disableProfile(ci::MUID muid);

    virtual std::vector<std::byte> profileDetailsInquired(ci::MUID muid,
                                                          ci::ProfileAtAddress profileAtAddress,
                                                          std::byte target) override;

    static ci::Profile MPE_PROFILE;
    static std::byte TARGET_FEATURES_SUPPORTED;

    String midiName_  {   };
    int manager_      { 0 };
    int members_      { 0 };
    
    std::byte supportsChannelResponse_  { 0x0 };
    std::byte supportsPitchBend_        { 0x1 };
    std::byte supportsChannelPressure_  { 0x1 };
    std::byte supportsThirdDimension_   { 0x1 };

    std::unique_ptr<ci::Device> ci_;
    std::unique_ptr<MidiInput> profileMidiIn_;
    std::unique_ptr<MidiOutput> profileMidiOut_;
    
    bool enabledProfile_    { false };
    ci::ProfileAtAddress enabledProfileAddress_;
};
