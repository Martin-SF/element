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

#include "engine/nodes/MidiMonitorNode.h"
#include "gui/nodes/MidiMonitorNodeEditor.h"
#include "gui/ViewHelpers.h"

namespace Element {

typedef ReferenceCountedObjectPtr<MidiMonitorNode> MidiMonitorNodePtr;

MidiMonitorNodeEditor::MidiMonitorNodeEditor (const Node& node)
    : NodeEditorComponent (node)
{

    midiMonitorLog.setBounds (0, 0, 320, 160);
    addAndMakeVisible (midiMonitorLog);

    setSize (320, 160);

    if (MidiMonitorNodePtr node = getNodeObjectOfType<MidiMonitorNode>())
    {
        node->addChangeListener (this);
    }
}

void MidiMonitorNodeEditor::changeListenerCallback (ChangeBroadcaster*)
{
    if (MidiMonitorNodePtr node = getNodeObjectOfType<MidiMonitorNode>())
    {
      MidiBuffer midi = node->toSendMidi;
      MidiBuffer::Iterator iter1 (midi);
      MidiMessage msg;
      int frame;

      while (iter1.getNextEvent (msg, frame))
        midiMonitorLog.addMessage(msg.getDescription());

      node->clearMidiLog();
    }
}

};
