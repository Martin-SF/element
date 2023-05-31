#pragma once

#include <element/juce/gui_basics.hpp>

namespace element {

class Context;
class Designer : public juce::Component {
public:
    Designer (Context& c);
    ~Designer();

    void resized() override;
    void paint (juce::Graphics&) override;
    void refresh();
    
private:
    class Content;
    std::unique_ptr<Content> content;
};

}
