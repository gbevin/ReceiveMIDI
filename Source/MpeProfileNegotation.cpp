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
    
// Profile Detail Inquiry Target
std::byte MpeProfileNegotiation::TARGET_FEATURES_SUPPORTED = std::byte(0x01);

MpeProfileNegotiation::MpeProfileNegotiation()
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
    ci_->addListener(*this);
}

void MpeProfileNegotiation::setProfileMidiName(const String& name)
{
    // remove all registered profiles
    for (int ch = 0x0; ch <= 0xE; ++ch)
    {
        ci_->getProfileHost()->removeProfile({ MPE_PROFILE, ci::ChannelAddress().withChannel((ci::ChannelInGroup)ch) });
    }
    
    // register profiles for all active manager channels
    if (manager_ == 0)
    {
        // support the MPE Profile on any channel as the manager channel, except on channel 16
        for (int ch = 0x0; ch <= 0xE; ++ch)
        {
            addMpeProfile(ch);
        }
    }
    else if (manager_ > 0)
    {
        addMpeProfile(manager_ - 1);
    }

    midiName_ = name;

#if (JUCE_LINUX || JUCE_MAC)
    if (profileMidiIn_ != nullptr)
    {
        profileMidiIn_->stop();
        profileMidiIn_ = nullptr;
    }
    profileMidiOut_ = nullptr;
    
    profileMidiIn_ = MidiInput::createNewDevice(midiName_, this);
    profileMidiIn_->start();
    
    profileMidiOut_ = MidiOutput::createNewDevice(midiName_);
    std::cout << "Responder " << muidToString(ci_->getMuid()) << " waiting for MPE Profile negotiation on " << (manager_ == 0 ? "all channels" : String("channel ") + String(manager_)) << std::endl;
#else
    std::cerr << "MPE Profile responder with virtual MIDI ports is not supported on Windows" << std::endl;
    JUCEApplicationBase::getInstance()->setApplicationReturnValue(EXIT_FAILURE);
#endif
}

void MpeProfileNegotiation::addMpeProfile(int manager)
{
    auto max_channels = 0xF - manager + 1;
    if (members_ > 0)
    {
        max_channels = std::min(members_ + 1, max_channels);
    }
    ci_->getProfileHost()->addProfile({ MPE_PROFILE, ci::ChannelAddress().withChannel((ci::ChannelInGroup)manager) }, max_channels);
}

void MpeProfileNegotiation::setManager(int manager)
{
    manager_ = manager;
}

void MpeProfileNegotiation::setMembers(int members)
{
    members_ = members;
}

void MpeProfileNegotiation::setSupportsChannelResponse(int flag)
{
    supportsChannelResponse_ = std::byte(flag);
}

void MpeProfileNegotiation::setSupportsPitchBend(int flag)
{
    supportsPitchBend_ = std::byte(flag);
}

void MpeProfileNegotiation::setSupportsChannelPressure(int flag)
{
    supportsChannelPressure_ = std::byte(flag);
}

void MpeProfileNegotiation::setSupportsThirdDimension(int flag)
{
    supportsThirdDimension_ = std::byte(flag);
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

void MpeProfileNegotiation::handleIncomingMidiMessage(MidiInput*, const MidiMessage& msg)
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
        if (enabledProfile_)
        {
            disableProfile(muid);
        }
        
        auto activated_channels = numChannels;
        if (members_ > 0)
        {
            activated_channels = std::min(activated_channels, members_ + 1);
        }
        activated_channels = std::min(activated_channels, 0xF - manager + 1);
        auto members = activated_channels - 1;
        std::cout << muidToString(muid) << " : MPE Profile enabled with manager channel " << (manager + 1) << " and " << members << " member channel" << (members > 1 ? "s" : "") << std::endl;
        host->setProfileEnablement(profileAtAddress, activated_channels);
        
        enabledProfile_ = true;
        enabledProfileAddress_ = profileAtAddress;
    }
    else
    {
        disableProfile(muid);
    }
}

void MpeProfileNegotiation::disableProfile(ci::MUID muid)
{
    std::cout << muidToString(muid) << " : MPE Profile disabled with manager channel " << ((int)enabledProfileAddress_.address.getChannel() + 1) << std::endl;
    ci_->getProfileHost()->setProfileEnablement(enabledProfileAddress_, -1);
    enabledProfile_ = false;
    enabledProfileAddress_ = ci::ProfileAtAddress();
}

std::vector<std::byte> MpeProfileNegotiation::profileDetailsInquired(ci::MUID muid, ci::ProfileAtAddress, std::byte target)
{
    std::cout << muidToString(muid) << " : MPE Profile details inquired for optional features" << std::endl;
    if (target == std::byte(TARGET_FEATURES_SUPPORTED))
    {
        return std::vector<std::byte> { supportsChannelResponse_, supportsPitchBend_, supportsChannelPressure_, supportsThirdDimension_ };
    }
    
    return std::vector<std::byte>();
};
