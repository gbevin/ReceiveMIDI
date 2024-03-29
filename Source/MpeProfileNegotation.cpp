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

#include "MpeProfileNegotiation.h"

#include "ApplicationState.h"

// MPE Profile ID
ci::Profile MpeProfileNegotiation::MPE_PROFILE = { std::byte(0x7E), std::byte(0x31), std::byte(0x00), std::byte(0x01), std::byte(0x01) };
    
MpeProfileNegotiation::MpeProfileNegotiation(ApplicationState* state)
{
    ci_ = std::make_unique<ci::Device>(ci::DeviceOptions()
                                       .withFeatures(ci::DeviceFeatures().withProfileConfigurationSupported())
                                       .withDeviceInfo( {
                                           ////////////////////////
                                           // IMPORTANT!
                                           //
                                           // This is Uwyn's SysEx ID, don't use for non-Uwyn products
                                           { std::byte(0x5B), std::byte(0x02), std::byte(0x00) },
                                           // Uwyn open-source product family
                                           { std::byte(0x01), std::byte(0x00) },
                                           // Uwyn ReceiveMIDI model number
                                           { std::byte(0x02), std::byte(0x00) },
                                           // Uwyn ReceiveMIDI revision
                                           { std::byte(0x01), std::byte(0x00), std::byte(0x00), std::byte(0x00) }} )
                                       .withOutputs({ this })
                                       .withProfileDelegate(this));
    // support the MPE profile on any channel as the manager channel, except on channel 16
    // we support all the possible channels upwards from the manager channel
    for (int ch = 0x0; ch <= 0xE; ++ch)
    {
        auto max_channels = 0xF - ch + 1;
        ci_->getProfileHost()->addProfile({ MPE_PROFILE, ci::ChannelAddress().withChannel((ci::ChannelInGroup)ch) }, max_channels);
    }
}

void MpeProfileNegotiation::setProfileMidiName(const String& name)
{
    midiName_ = name;
    
    if (profileMidiIn_ != nullptr)
    {
        profileMidiIn_->stop();
        profileMidiIn_ = nullptr;
    }
    profileMidiOut_ = nullptr;
    
    profileMidiIn_ = MidiInput::createNewDevice(midiName_, this);
    profileMidiIn_->start();
    
    profileMidiOut_ = MidiOutput::createNewDevice(midiName_);
    std::cout << "Responder " << muidToString(ci_->getMuid()) << " waiting for MPE profile negotiation on " << (manager_ == 0 ? "all channels" : String("channel ") + String(manager_)) << std::endl;
}

void MpeProfileNegotiation::setManager(int manager)
{
    manager_ = manager;
}

void MpeProfileNegotiation::setMembers(int members)
{
    members_ = members;
}

void MpeProfileNegotiation::processMessage(ump::BytesOnGroup umsg)
{
    if (midiName_.isEmpty())
    {
        std::cerr << "Missing MIDI port name for CI" << std::endl;
        return;
    }
    
    auto msg = MidiMessage::createSysExMessage(umsg.bytes);
    profileMidiOut_->sendMessageNow(msg);
}

std::string MpeProfileNegotiation::muidToString(ci::MUID muid)
{
    std::stringstream s;
    s << "MUID 0x" << std::hex << std::setfill('0') << std::setw(8) << muid.get() << std::dec;
    return s.str();
}

void MpeProfileNegotiation::handleIncomingMidiMessage(MidiInput* source, const MidiMessage& msg)
{
    if (msg.isSysEx())
    {
        // we don't have any other application threads going on, so this is safe
        // even though ci::Device is not thread safe, in a proper application
        // this should done asynchronously on the message thread
        ci_->processMessage({0, msg.getSysExDataSpan()});
    }
}

void MpeProfileNegotiation::profileEnablementRequested(ci::MUID muid, ci::ProfileAtAddress profileAtAddress, int numChannels, bool enabled)
{
    auto host = ci_->getProfileHost();
    auto manager = (int)profileAtAddress.address.getChannel();
    if (enabled && (manager_ == 0 || manager == manager_ - 1))
    {
        auto activated_channels = numChannels;
        if (members_ > 0)
        {
            activated_channels = std::min(activated_channels, members_ + 1);
        }
        activated_channels = std::min(activated_channels, 0xF - manager + 1);
        auto members = activated_channels - 1;
        std::cout << muidToString(muid) << " : MPE profile enabled with manager channel " << (manager + 1) << " and " << members << " member channel" << (members > 1 ? "s" : "") << std::endl;
        host->setProfileEnablement(profileAtAddress, activated_channels);
    }
    else
    {
        std::cout << muidToString(muid) << " : MPE profile disabled with manager channel " << (manager + 1) << std::endl;
        host->setProfileEnablement(profileAtAddress, -1);
    }
}
