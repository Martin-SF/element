// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/ui/navigation.hpp>
#include <element/ui/style.hpp>
#include <element/engine.hpp>

#include "nodes/audiofileplayer.hpp"

#include "ui/buttons.hpp"
#include "ui/datapathbrowser.hpp"
#include "ui/viewhelpers.hpp"
#include "ui/audioiopanelview.hpp"
#include "ui/sessiontreepanel.hpp"
#include "ui/pluginspanelview.hpp"
#include "ui/filecombobox.hpp"

#include "utils.hpp"

namespace element {

class AudioFilePlayerEditor;

class AudioFilePlayerTransport : public juce::Component
{
public:
    AudioFilePlayerTransport()
    {
        addAndMakeVisible (play);
        addAndMakeVisible (stop);
        addAndMakeVisible (cont);
        addAndMakeVisible (rewind);

        setSize (22 * 4 + 2 * 3, 18);
    }

    ~AudioFilePlayerTransport()
    {
        play.onClick = nullptr;
        stop.onClick = nullptr;
        cont.onClick = nullptr;
        rewind.onClick = nullptr;
    }

    void resized() override
    {
        auto r = getLocalBounds();
        std::vector<Component*> comps = {
            &play, &stop, &cont, &rewind
        };

        for (auto* c : comps)
        {
            c->setBounds (r.removeFromLeft (_buttonSize));
            r.removeFromLeft (2);
        }
    }

    int requiredWidth()
    {
        const int btnW = std::max (14, _buttonSize), nbtn = 4, pad = 2;
        return nbtn * btnW + (nbtn - 1) * pad;
    }

    void setButtonSize (int newSize)
    {
        _buttonSize = std::max (14, newSize);
        setSize (requiredWidth(), std::max (14, _buttonSize - 6));
    }

private:
    friend class AudioFilePlayerEditor;
    int _buttonSize = 22;
    PlayButton play { "Play" };
    StopButton stop { "Stop" };
    ContinueButton cont { "Continue" };
    SeekZeroButton rewind { "Seek to Zero" };
};

class AudioFilePlayerEditor : public AudioProcessorEditor,
                              public FileComboBoxListener,
                              public ChangeListener,
                              public DragAndDropTarget,
                              public FileDragAndDropTarget,
                              public Timer
{
public:
    AudioFilePlayerEditor (AudioFilePlayerNode& o)
        : AudioProcessorEditor (&o),
          processor (o)
    {
        setOpaque (true);
        chooser.reset (new FileComboBox ("Audio File",
                                         File(),
                                         false,
                                         false,
                                         false,
                                         o.getWildcard(),
                                         String(),
                                         TRANS ("Select Audio File")));
        addAndMakeVisible (chooser.get());
        chooser->setShowFullPathName (false);

        addAndMakeVisible (watchButton);
        watchButton.setIcon (Icon (getIcons().fasFolderOpen, Colours::black));

        addAndMakeVisible (transport);

        addAndMakeVisible (playButton);
        playButton.setButtonText ("Play");

        addAndMakeVisible (loopToggle);
        loopToggle.setButtonText ("Loop");

        addAndMakeVisible (startStopContinueToggle);
        startStopContinueToggle.setButtonText (TRANS ("MIDI S/S/C"));

        addAndMakeVisible (hostToggle);
        hostToggle.setClickingTogglesState (true);
        hostToggle.setButtonText (TRANS ("Host"));

        addAndMakeVisible (position);
        position.setSliderStyle (Slider::LinearBar);
        position.setRange (0.0, 1.0, 0.001);
        position.setTextBoxIsEditable (false);

        addAndMakeVisible (volume);
        volume.setSliderStyle (position.getSliderStyle());
        volume.setRange (-60.0, 12.0, 0.1);
        volume.setTextBoxIsEditable (false);

        stabilizeComponents();
        bindHandlers();

        setSize (356, 175);
        startTimer (1001);
    }

    ~AudioFilePlayerEditor() noexcept
    {
        stopTimer();
        unbindHandlers();
        chooser = nullptr;
    }

    void addRecentsFrom (const File& recentsDir, bool recursive = true)
    {
        if (recentsDir.isDirectory())
        {
            for (DirectoryEntry entry : RangedDirectoryIterator (recentsDir, recursive, processor.getWildcard()))
            {
                if (entry.getFile().isDirectory())
                    continue;
                chooser->addRecentlyUsedFile (entry.getFile());
            }

            sortRecents();
        }
    }

    void timerCallback() override { stabilizeComponents(); }
    void changeListenerCallback (ChangeBroadcaster*) override { stabilizeComponents(); }

    void stabilizeComponents()
    {
        if (processor.getWatchDir().isDirectory())
            if (chooser->getRecentlyUsedFilenames().isEmpty())
                addRecentsFrom (processor.getWatchDir());

        if (chooser->getCurrentFile() != processor.getAudioFile())
            if (processor.getAudioFile().existsAsFile())
                chooser->setCurrentFile (processor.getAudioFile(), dontSendNotification);

        transport.play.setToggleState (processor.getPlayer().isPlaying(), dontSendNotification);

        loopToggle.setToggleState (processor.isLooping(), dontSendNotification);

        if (! draggingPos)
        {
            if (processor.getPlayer().getLengthInSeconds() > 0.0)
            {
                position.setValue (
                    processor.getPlayer().getCurrentPosition() / processor.getPlayer().getLengthInSeconds(),
                    dontSendNotification);
            }
            else
            {
                position.setValue (position.getMinimum(), dontSendNotification);
            }
            position.updateText();
        }

        volume.setValue (
            (double) Decibels::gainToDecibels ((double) processor.getPlayer().getGain(), (double) volume.getMinimum()),
            dontSendNotification);

        startStopContinueToggle.setToggleState (processor.respondsToStartStopContinue(),
                                                dontSendNotification);
        hostToggle.setToggleState (processor.hostSyncEnabled(), dontSendNotification);
    }

    void fileComboBoxChanged (FileComboBox*) override
    {
        const auto f1 = chooser->getCurrentFile();
        const auto f2 = processor.getAudioFile();
        if (! f1.isDirectory() && f1 != f2)
            processor.openFile (chooser->getCurrentFile());
    }

    void resized() override
    {
        auto r (getLocalBounds().reduced (4));
        auto r2 = r.removeFromTop (18);

        watchButton.setBounds (r2.removeFromRight (22));
        chooser->setBounds (r2);

        r.removeFromTop (4);
        transport.setButtonSize (_transportButtonSize);
        transport.setBounds (r.removeFromTop (77).withSizeKeepingCentre (transport.getWidth(), transport.getHeight()));

        r.removeFromTop (4);
        volume.setBounds (r.removeFromTop (18));
        r.removeFromTop (4);
        position.setBounds (r.removeFromTop (18));
        r.removeFromTop (4);
        r = r.removeFromTop (18);

        std::vector<ToggleButton*> toggles {
            &loopToggle, &hostToggle, &startStopContinueToggle
        };
        for (auto* t : toggles)
            t->setBounds (r.removeFromLeft (getWidth() / (int) toggles.size()));
    }

    void paint (Graphics& g) override
    {
        g.fillAll (Colors::widgetBackgroundColor);
    }

    //=========================================================================
    bool isInterestedInDragSource (const SourceDetails& details) override
    {
        if (details.description.toString() == "ccNavConcertinaPanel")
            return true;
        return false;
    }

    void itemDropped (const SourceDetails& details) override {}
#if 0
    virtual void itemDragEnter (const SourceDetails& dragSourceDetails);
    virtual void itemDragMove (const SourceDetails& dragSourceDetails);
    virtual void itemDragExit (const SourceDetails& dragSourceDetails);
    virtual bool shouldDrawDragImageWhenOver();
#endif

    //=========================================================================
    bool isInterestedInFileDrag (const StringArray& files) override
    {
        if (! File::isAbsolutePath (files[0]))
            return false;
        return processor.canLoad (File (files[0]));
    }

    void filesDropped (const StringArray& files, int x, int y) override
    {
        ignoreUnused (x, y);
        processor.openFile (File (files[0]));
    }

#if 0
    virtual void fileDragEnter (const StringArray& files, int x, int y);
    virtual void fileDragExit (const StringArray& files);
#endif

private:
    AudioFilePlayerNode& processor;
    std::unique_ptr<FileComboBox> chooser;
    AudioFilePlayerTransport transport;
    Slider position;
    Slider volume;
    TextButton playButton;
    IconButton watchButton;
    ToggleButton startStopContinueToggle,
        hostToggle,
        loopToggle;
    Atomic<int> startStopContinue { 0 };
    SignalConnection stateRestoredConnection;

    bool draggingPos = false;

    int _transportButtonSize = 32;

    void sortRecents()
    {
        auto names = chooser->getRecentlyUsedFilenames();
        names.sort (false);
        chooser->setRecentlyUsedFilenames (names);
    }

    void bindHandlers()
    {
        processor.getPlayer().addChangeListener (this);
        stateRestoredConnection = processor.restoredState.connect (std::bind (
            &AudioFilePlayerEditor::onStateRestored, this));

        chooser->addListener (this);
        watchButton.onClick = [this]() {
            FileChooser fc ("Select a folder to watch", File(), "*", true, false, nullptr);
            if (fc.browseForDirectory())
            {
                processor.setWatchDir (fc.getResult());
                addRecentsFrom (processor.getWatchDir());
            }
        };

        transport.play.onClick = [this]() {
            int index = AudioFilePlayerNode::Playing;
            if (auto* playing = dynamic_cast<AudioParameterBool*> (processor.getParameters()[index]))
            {
                *playing = true;
                stabilizeComponents();
            }
        };

        transport.stop.onClick = [this]() {
            int index = AudioFilePlayerNode::Playing;
            if (auto* playing = dynamic_cast<AudioParameterBool*> (processor.getParameters()[index]))
            {
                *playing = false;
                stabilizeComponents();
            }
        };

        transport.cont.onClick = transport.play.onClick;

        transport.rewind.onClick = [this]() {
            processor.getPlayer().setPosition (0.0);
        };

        playButton.onClick = transport.play.onClick;

        loopToggle.onClick = [this]() {
            processor.setLooping (! processor.isLooping());
            stabilizeComponents();
        };

        volume.onValueChange = [this]() {
            int index = AudioFilePlayerNode::Volume;
            if (auto* const param = dynamic_cast<AudioParameterFloat*> (processor.getParameters()[index]))
            {
                *param = static_cast<float> (volume.getValue());
                stabilizeComponents();
            }
        };

        position.onDragStart = [this]() { draggingPos = true; };
        position.onDragEnd = [this]() {
            const auto newPos = position.getValue() * processor.getPlayer().getLengthInSeconds();
            processor.getPlayer().setPosition (newPos);
            draggingPos = false;
            stabilizeComponents();
        };

        position.textFromValueFunction = [this] (double value) -> String {
            const double posInMinutes = (value * processor.getPlayer().getLengthInSeconds()) / 60.0;
            return Util::minutesToString (posInMinutes);
        };

        startStopContinueToggle.onClick = [this]() {
            processor.setRespondToStartStopContinue (
                startStopContinueToggle.getToggleState() ? 1 : 0);
            startStopContinueToggle.setToggleState (
                processor.respondsToStartStopContinue(), dontSendNotification);
        };

        hostToggle.onClick = [this]() {
            processor.enableHostSync (hostToggle.getToggleState());
        };
    }

    void unbindHandlers()
    {
        stateRestoredConnection.disconnect();

        transport.play.onClick = nullptr;
        transport.stop.onClick = nullptr;
        transport.cont.onClick = nullptr;
        transport.rewind.onClick = nullptr;

        playButton.onClick = nullptr;
        loopToggle.onClick = nullptr;
        position.onDragStart = nullptr;
        position.onDragEnd = nullptr;
        position.textFromValueFunction = nullptr;
        volume.onValueChange = nullptr;
        startStopContinueToggle.onClick = nullptr;
        hostToggle.onClick = nullptr;
        processor.getPlayer().removeChangeListener (this);
        chooser->removeListener (this);
        watchButton.onClick = nullptr;
    }

    void onStateRestored()
    {
        auto watchDir = processor.getWatchDir();
        if (! watchDir.exists() || ! watchDir.isDirectory())
            return;
        addRecentsFrom (watchDir, true);
    }
};

AudioFilePlayerNode::AudioFilePlayerNode()
    : BaseProcessor (BusesProperties()
                         .withOutput ("Main", AudioChannelSet::stereo(), true))
{
    addLegacyParameter (playing = new AudioParameterBool ("playing", "Playing", false));
    addLegacyParameter (slave = new AudioParameterBool ("slave", "Slave", false));
    addLegacyParameter (volume = new AudioParameterFloat ("volume", "Volume", -60.f, 12.f, 0.f));
    addLegacyParameter (looping = new AudioParameterBool ("loop", "Loop", false));

    for (auto* const param : getParameters())
        param->addListener (this);
}

AudioFilePlayerNode::~AudioFilePlayerNode()
{
    for (auto* const param : getParameters())
        param->removeListener (this);
    clearPlayer();
    playing = nullptr;
    slave = nullptr;
    volume = nullptr;
}

void AudioFilePlayerNode::setRespondToStartStopContinue (bool respond)
{
    midiStartStopContinue.set (respond ? 1 : 0);
}

bool AudioFilePlayerNode::respondsToStartStopContinue() const
{
    return midiStartStopContinue.get() == 1;
}

void AudioFilePlayerNode::enableHostSync (bool sync)
{
    juce::ScopedLock sl (getCallbackLock());
    *slave = sync;
}

bool AudioFilePlayerNode::hostSyncEnabled() const noexcept
{
    juce::ScopedLock sl (getCallbackLock());
    return *slave;
}

void AudioFilePlayerNode::fillInPluginDescription (PluginDescription& desc) const
{
    desc.name = getName();
    desc.fileOrIdentifier = EL_NODE_ID_AUDIO_FILE_PLAYER;
    desc.descriptiveName = "A single audio file player";
    desc.numInputChannels = 0;
    desc.numOutputChannels = 2;
    desc.hasSharedContainer = false;
    desc.isInstrument = false;
    desc.manufacturerName = EL_NODE_FORMAT_AUTHOR;
    desc.pluginFormatName = "Element";
    desc.version = "1.0.0";
    desc.uniqueId = EL_NODE_UID_AUDIO_FILE_PLAYER;
}

void AudioFilePlayerNode::clearPlayer()
{
    player.setSource (nullptr);
    if (reader)
        reader = nullptr;
    *playing = player.isPlaying();
}

void AudioFilePlayerNode::openFile (const File& file)
{
    if (file == audioFile)
        return;
    if (auto* newReader = formats.createReaderFor (file))
    {
        clearPlayer();
        reader.reset (new AudioFormatReaderSource (newReader, true));
        audioFile = file;
        player.setSource (reader.get(), 1024 * 8, &thread, newReader->sampleRate, 2);

        ScopedLock sl (getCallbackLock());
        reader->setLooping (*looping);
        player.setLooping (*looping);
    }
}

void AudioFilePlayerNode::prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock)
{
    thread.startThread();
    formats.registerBasicFormats();
    player.prepareToPlay (maximumExpectedSamplesPerBlock, sampleRate);

    if (reader)
    {
        double readerSampleRate = sampleRate;
        if (auto* fmtReader = reader->getAudioFormatReader())
            readerSampleRate = fmtReader->sampleRate;

        reader->setLooping (*looping);
        player.setLooping (*looping);
        player.setSource (reader.get(), 1024 * 8, &thread, readerSampleRate, 2);
        player.setPosition (jmax (0.0, lastTransportPos));
        if (wasPlaying)
            player.start();
    }
    else
    {
        clearPlayer();
    }
}

void AudioFilePlayerNode::releaseResources()
{
    lastTransportPos = player.getCurrentPosition();
    wasPlaying = player.isPlaying();

    player.stop();
    player.releaseResources();
    player.setSource (nullptr);
    formats.clearFormats();
    thread.stopThread (14);
}

void AudioFilePlayerNode::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    const auto nframes = buffer.getNumSamples();
    for (int c = buffer.getNumChannels(); --c >= 0;)
        buffer.clear (c, 0, nframes);

    ScopedLock sl (getCallbackLock());
    const bool hostSync = *slave;
    if (hostSync)
    {
        if (auto* const playhead = getPlayHead())
        {
            auto pos = playhead->getPosition();
            if (pos)
            {
                if (pos->getTimeInSamples() == 0 && player.getCurrentPosition() != 0.0)
                {
                    player.setPosition (0.0);
                }

                if (player.isPlaying() != pos->getIsPlaying())
                {
                    if (pos->getIsPlaying())
                        midiPlayState.set (Continue);
                    else
                        midiPlayState.set (Stop);

                    triggerAsyncUpdate();
                }
            }
        }
    }

    MidiMessage msg;
    int start = 0;
    AudioSourceChannelInfo info;
    info.buffer = &buffer;

    if (! hostSync && midiStartStopContinue.get() == 1)
    {
        for (auto m : midi)
        {
            msg = m.getMessage();
            info.startSample = start;
            info.numSamples = m.samplePosition - start;
            player.getNextAudioBlock (info);

            if (msg.isMidiStart())
            {
                midiPlayState.set (Start);
                triggerAsyncUpdate();
            }
            else if (msg.isMidiContinue())
            {
                midiPlayState.set (Continue);
                triggerAsyncUpdate();
            }
            else if (msg.isMidiStop())
            {
                midiPlayState.set (Stop);
                triggerAsyncUpdate();
            }

            start = m.samplePosition;
        }
    }

    if (start < nframes)
    {
        info.startSample = start;
        info.numSamples = nframes - start;
        player.getNextAudioBlock (info);
    }

    midi.clear();
}

void AudioFilePlayerNode::setLooping (const bool shouldLoop)
{
    jassert (looping != nullptr);
    *looping = shouldLoop;
}

bool AudioFilePlayerNode::isLooping() const
{
    return *looping;
}

void AudioFilePlayerNode::handleAsyncUpdate()
{
    switch (midiPlayState.get())
    {
        case Start: {
            player.setPosition (0.0);
            player.start();
        }
        break;

        case Stop: {
            player.stop();
        }
        break;

        case Continue: {
            player.start();
        }
        break;

        case None:
        default:
            break;
    }

    midiPlayState.set (None);
}

AudioProcessorEditor* AudioFilePlayerNode::createEditor()
{
    return new AudioFilePlayerEditor (*this);
}

void AudioFilePlayerNode::getStateInformation (juce::MemoryBlock& destData)
{
    ValueTree state (tags::state);
    state.setProperty ("audioFile", audioFile.getFullPathName(), nullptr)
        .setProperty ("playing", (bool) *playing, nullptr)
        .setProperty ("slave", (bool) *slave, nullptr)
        .setProperty ("loop", (bool) *looping, nullptr)
        .setProperty ("midiStartStopContinue", midiStartStopContinue.get() == 1, nullptr);

    if (watchDir.exists())
        state.setProperty ("watchDir", watchDir.getFullPathName(), nullptr);

    MemoryOutputStream stream (destData, false);
    state.writeToStream (stream);
}

void AudioFilePlayerNode::setStateInformation (const void* data, int sizeInBytes)
{
    const auto state = ValueTree::readFromData (data, (size_t) sizeInBytes);
    if (state.isValid())
    {
        if (File::isAbsolutePath (state["audioFile"].toString()))
            openFile (File (state["audioFile"].toString()));
        *playing = (bool) state.getProperty ("playing", false);
        *slave = (bool) state.getProperty ("slave", false);
        *looping = (bool) state.getProperty ("loop", true);
        midiStartStopContinue.set ((bool) state.getProperty ("midiStartStopContinue", false) ? 1 : 0);
        if (state.hasProperty ("watchDir"))
        {
            auto watchPath = state["watchDir"].toString();
            if (File::isAbsolutePath (watchPath))
                watchDir = File (watchPath);
        }
        restoredState();
    }
}

void AudioFilePlayerNode::parameterValueChanged (int parameter, float newValue)
{
    ignoreUnused (newValue);

    switch (parameter)
    {
        case Playing: {
            if (*playing)
                player.start();
            else
                player.stop();
        }
        break;

        case Slave: {
            // noop
        }
        break;

        case Volume: {
            player.setGain (Decibels::decibelsToGain (volume->get(), volume->range.start));
        }
        break;

        case Looping: {
            if (reader != nullptr)
            {
                player.setLooping (*looping);
                reader->setLooping (*looping);
            }
        }
        break;
    }
}

void AudioFilePlayerNode::parameterGestureChanged (int parameterIndex, bool gestureIsStarting)
{
    ignoreUnused (parameterIndex, gestureIsStarting);
}

bool AudioFilePlayerNode::isBusesLayoutSupported (const BusesLayout& layout) const
{
    // only one main output bus supported. stereo or mono
    if (layout.inputBuses.size() > 0 || layout.outputBuses.size() > 1)
        return false;
    return layout.getMainOutputChannelSet() == AudioChannelSet::stereo() || layout.getMainOutputChannelSet() == AudioChannelSet::mono();
}

} // namespace element
