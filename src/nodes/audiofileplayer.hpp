// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include "nodes/baseprocessor.hpp"
#include <element/signals.hpp>

namespace element {

class AudioFilePlayerNode : public BaseProcessor,
                            public AudioProcessorParameter::Listener,
                            public AsyncUpdater
{
public:
    enum Parameters
    {
        Playing = 0,
        Slave,
        Volume,
        Looping
    };
    enum MidiPlayState
    {
        None = 0,
        Start,
        Stop,
        Continue
    };

    AudioFilePlayerNode();
    virtual ~AudioFilePlayerNode();

    AudioFormatManager& getAudioFormatManager() { return formats; }
    void setWatchDir (const File& newWatchDir)
    {
        watchDir = newWatchDir;
        jassert (newWatchDir.isDirectory());
    }
    File getWatchDir() const { return watchDir; }

    void handleAsyncUpdate() override;

    void setLooping (const bool shouldLoop);
    bool isLooping() const;

    void openFile (const File& file);
    const File& getAudioFile() const { return audioFile; }
    String getWildcard() const { return formats.getWildcardForAllFormats(); }

    bool canLoad (const File& file)
    {
        std::unique_ptr<AudioFormatReader> reader (formats.createReaderFor (file));
        return reader != nullptr;
    }

    void fillInPluginDescription (PluginDescription& desc) const override;

    void setRespondToStartStopContinue (bool);
    bool respondsToStartStopContinue() const;

    void enableHostSync (bool sync);
    bool hostSyncEnabled() const noexcept;

    const String getName() const override { return "Audio File Player"; }
    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

    bool canAddBus (bool isInput) const override
    {
        ignoreUnused (isInput);
        return false;
    }
    bool canRemoveBus (bool isInput) const override
    {
        ignoreUnused (isInput);
        return false;
    }

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool supportsMPE() const override { return false; }
    bool isMidiEffect() const override { return false; }

    int getNumPrograms() override { return 1; };
    int getCurrentProgram() override { return 0; };
    void setCurrentProgram (int index) override { ignoreUnused (index); };
    const String getProgramName (int index) override
    {
        ignoreUnused (index);
        return getName();
    }
    void changeProgramName (int index, const String& newName) override { ignoreUnused (index, newName); }

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void parameterValueChanged (int parameterIndex, float newValue) override;
    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override;

    AudioTransportSource& getPlayer() { return player; }

    Signal<void()> restoredState;

protected:
    bool isBusesLayoutSupported (const BusesLayout&) const override;

#if 0
    // Audio Processor Template
    
    virtual StringArray getAlternateDisplayNames() const;
    virtual void processBlock (AudioBuffer<double>& buffer, idiBuffer& midiMessages);
    virtual void processBlockBypassed (AudioBuffer<float>& buffer, MidiBuffer& midiMessages);
    virtual void processBlockBypassed (AudioBuffer<double>& buffer, MidiBuffer& midiMessages);
    
    virtual bool supportsDoublePrecisionProcessing() const;
    
    virtual void reset();
    virtual void setNonRealtime (bool isNonRealtime) noexcept;
    
    virtual void getCurrentProgramStateInformation (juce::MemoryBlock& destData);
    virtual void setCurrentProgramStateInformation (const void* data, int sizeInBytes);
    
    virtual void numChannelsChanged();
    virtual void numBusesChanged();
    virtual void processorLayoutsChanged();
    
    virtual void addListener (AudioProcessorListener* newListener);
    virtual void removeListener (AudioProcessorListener* listenerToRemove);
    virtual void setPlayHead (AudioPlayHead* newPlayHead);
    
    virtual void updateTrackProperties (const TrackProperties& properties);

protected:
    virtual bool canApplyBusesLayout (const BusesLayout& layouts) const     { return isBusesLayoutSupported (layouts); }
    virtual bool canApplyBusCountChange (bool isInput, bool isAddingBuses, BusProperties& outNewBusProperties);
#endif

private:
    friend class AudioFilePlayerEditor;

    TimeSliceThread thread { "MediaPlayer" };
    std::unique_ptr<AudioFormatReaderSource> reader;
    AudioFormatManager formats;
    AudioTransportSource player;

    AudioParameterBool* slave { nullptr };
    AudioParameterBool* playing { nullptr };
    AudioParameterFloat* volume { nullptr };
    AudioParameterBool* looping { nullptr };

    File audioFile;
    Atomic<int> midiStartStopContinue;
    Atomic<int> midiPlayState { None };

    bool wasPlaying { false };
    double lastTransportPos { 0.0 };

    File watchDir;

    void clearPlayer();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioFilePlayerNode)
};

} // namespace element
