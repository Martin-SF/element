/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#pragma once

#include "engine/nodes/BaseProcessor.h"
#include "engine/GraphNode.h"
#include "scripting/LuaState.h"

namespace Element {

struct LuaMidiBuffer;
struct LuaMidiPipe;

class LuaNode : public GraphNode
{
public:
    explicit LuaNode();
    virtual ~LuaNode();
    
    void fillInPluginDescription (PluginDescription& desc);
    void prepareToRender (double sampleRate, int maxBufferSize) override;
    void releaseResources() override;
    void render (AudioSampleBuffer& audio, MidiPipe& midi) override;
    void setState (const void* data, int size) override;
    void getState (MemoryBlock& block) override;

    const String& getScript() const { return script; }
    const String& getDraftScript() const { return draftScript; }
    void setDraftScript (const String& draft) { draftScript = draft; }
    bool hasChanges() const { return script.hashCode64() != draftScript.hashCode64(); }

protected:
    inline bool wantsMidiPipe() const override { return true; }
    
    inline void createPorts() override
    {
        if (createdPorts)
            return;

        ports.clearQuick();
        ports.add (PortType::Midi, 0, 0, "midi_in", "MIDI In", true);
        ports.add (PortType::Midi, 1, 0, "midi_out", "MIDI Out", false);
        createdPorts = true;
    }

private:
    bool createdPorts = false;
    LuaState state;
    String script, draftScript;

    LuaMidiBuffer* luaMidiBuffer { nullptr };
    LuaMidiPipe* luaMidiPipe { nullptr };

    int renderRef { - 1 },
        midiPipeRef { - 1},
        audioRef { -1 };
};

}
