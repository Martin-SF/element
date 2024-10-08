// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/version.hpp>

#include "ui/aboutscreen.hpp"
#include "appinfo.hpp"

// resources
#include "ui/res.hpp"

#define EL_LICENSE_TEXT                                                        \
    "Copyright (C) 2014-@0@  Kushview, LLC.  All rights reserved.\r\n\r\n"     \
                                                                               \
    "This program is free software; you can redistribute it and/or modify\r\n" \
    "it under the terms of the GNU General Public License as published by\r\n" \
    "the Free Software Foundation; either version 3 of the License, or\r\n"    \
    "(at your option) any later version.\r\n\r\n"                              \
                                                                               \
    "This program is distributed in the hope that it will be useful,\r\n"      \
    "but WITHOUT ANY WARRANTY; without even the implied warranty of\r\n"       \
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\r\n"        \
    "GNU General Public License for more details.\r\n\r\n"                     \
                                                                               \
    "You should have received a copy of the GNU General Public License\r\n"    \
    "along with this program; if not, write to the Free Software\r\n"          \
    "Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\r\n"

namespace element {

namespace detail {

static String licenseText()
{
    auto year = Time::getCurrentTime().getYear();
    return String (EL_LICENSE_TEXT).replace ("@0@", String (year));
}

static StringArray developers()
{
    const auto str = String (res::developers_txt, res::developers_txtSize);
    StringArray devs;
    devs.addTokens (str, "\n", {});
    devs.removeDuplicates (false);
    devs.removeEmptyStrings();
    return devs;
}

static void setupMonoEditor (TextEditor& text)
{
    text.setCaretVisible (false);
    text.setMultiLine (true, false);
    text.setFont (Font (Font::getDefaultMonospacedFontName(), 13.f, Font::plain));
    text.setReadOnly (true);
}

} // namespace detail

class AboutCreditsPanel : public Component
{
public:
    AboutCreditsPanel()
    {
        setSize (100, 24);
    }

    void addSection (const String& title, const StringArray& names)
    {
        auto* section = sections.add (new Section());
        section->title.setText (title, dontSendNotification);
        addAndMakeVisible (section->title);
        for (const auto& name : names)
        {
            auto* nameLabel = section->names.add (new Label (name, name));
            nameLabel->setFont (13.f);
            addAndMakeVisible (nameLabel);
        }

        setSize (getWidth(), getTotalHeight());
        resized();
    }

    void resized() override
    {
        int y = 0;
        for (auto* section : sections)
        {
            section->title.setBounds (0, y, getWidth(), titleHeight);
            y += titleHeight;
            for (auto* name : section->names)
            {
                name->setBounds (8, y, getWidth(), nameHeight);
                y += nameHeight;
            }
        }
    }

private:
    struct Section
    {
        Label title;
        OwnedArray<Label> names;
    };

    OwnedArray<Section> sections;
    int titleHeight = 24;
    int nameHeight = 20;
    int getTotalHeight()
    {
        auto size = sections.size() * titleHeight;
        for (auto* section : sections)
            for (auto* name : section->names)
            {
                size += nameHeight;
                ignoreUnused (name);
            }
        return size;
    }
};

class AboutCreditsComponent : public Component
{
public:
    AboutCreditsComponent()
    {
        addAndMakeVisible (view);
        view.setViewedComponent (&panel, false);
    }

    AboutCreditsPanel& getPanel() { return panel; }

    void resized() override
    {
        panel.setSize (getWidth() - 14, panel.getHeight());
        view.setBounds (getLocalBounds());
    }

private:
    AboutCreditsPanel panel;
    Viewport view;
};

//=============================================================================

class LicenseTextComponent : public Component
{
public:
    LicenseTextComponent()
    {
        addAndMakeVisible (text);
        detail::setupMonoEditor (text);
        setLicenseText (detail::licenseText());
    }

    void setLicenseText (const String& newText)
    {
        text.setText (newText);
    }

    void resized() override
    {
        text.setBounds (getLocalBounds());
    }

private:
    TextEditor text;
};

//=============================================================================

class AckTextComponent : public Component
{
public:
    AckTextComponent()
    {
        addAndMakeVisible (text);
        detail::setupMonoEditor (text);
        text.setText (String (res::acknowledgements_txt, res::acknowledgements_txtSize));
    }

    void resized() override
    {
        text.setBounds (getLocalBounds());
    }

private:
    TextEditor text;
};

//=============================================================================

AboutScreen::AboutScreen()
{
    setOpaque (true);

    elementLogo = Drawable::createFromImageData (
        res::icon_png, res::icon_pngSize);

    addAndMakeVisible (titleLabel);
    titleLabel.setJustificationType (Justification::centred);
    titleLabel.setFont (Font (34.0f, Font::FontStyleFlags::bold));

    auto buildDate = Time::getCompilationDate();
    addAndMakeVisible (versionLabel);
    versionLabel.setText (String ("Version: ") + Version::withGitHash()
                              + "\nBuild date: " + String (buildDate.getDayOfMonth())
                              + " " + Time::getMonthName (buildDate.getMonth(), true)
                              + " " + String (buildDate.getYear()),
                          dontSendNotification);

    versionLabel.setJustificationType (Justification::centred);
    versionLabel.setFont (Font (13.f));

    addAndMakeVisible (copyrightLabel);
    copyrightLabel.setJustificationType (Justification::centred);
    copyrightLabel.setFont (Font (13.f));
    String copyrightText = "Copyright ";
    copyrightText << String (CharPointer_UTF8 ("\xc2\xa9")) << " XXX Kushview, LLC.";
    copyrightLabel.setText (copyrightText.replace ("XXX", String (buildDate.getYear())),
                            dontSendNotification);

    addAndMakeVisible (aboutButton);
    aboutButton.setTooltip ({});
    aboutButton.setColour (HyperlinkButton::textColourId, Colors::toggleBlue);

    addAndMakeVisible (tabs);
    tabs.setTabBarDepth (24);
    tabs.setOutline (0);
    const auto tabc = findColour (TextEditor::backgroundColourId);

    auto* authors = new AboutCreditsComponent();
    authors->getPanel().addSection ("Lead Developer", { "Michael Fisher (mfisher31)" });
    authors->getPanel().addSection ("Contributers", detail::developers());
    tabs.addTab ("Authors", tabc, authors, true);

    // auto* donors = new AboutCreditsComponent();
    // donors->getPanel().addSection ("Gold Sponsors", { "None" });
    // donors->getPanel().addSection ("Silver Sponsors", { "None" });
    // donors->getPanel().addSection ("Gold Backers", { "None" });
    // donors->getPanel().addSection ("Sponsors", { "Davide Anselmi", "Greg Gibbs", "Kent Kingery", "Michael Kıral" });

    // tabs.addTab ("Donors", tabc, donors, true);
    tabs.addTab ("License", tabc, new LicenseTextComponent(), true);
    tabs.addTab ("Credits", tabc, new AckTextComponent(), true);

    addAndMakeVisible (copyVersionButton);
    copyVersionButton.setButtonText (TRANS ("Copy"));
    copyVersionButton.onClick = [this]() { copyVersion(); };

    setSize (510, 330);

    AboutInfo i;
    i.title = EL_APP_NAME;
    i.copyright << "Copyright " << String (CharPointer_UTF8 ("\xc2\xa9")) << " XXX Kushview, LLC.";
    i.copyright = i.copyright.replace ("XXX", String (buildDate.getYear()));
    i.version = ("Version: ") + Version::withGitHash();
    i.version << " (build " << EL_BUILD_NUMBER << ")";
    setAboutInfo (i);
}

void AboutScreen::setAboutInfo (const AboutInfo& details)
{
    info = details;
    updateAboutInfo();
}

void AboutScreen::resized()
{
    auto bounds = getLocalBounds();
    elementLogoBounds.setBounds (14, 14, 72, 72);
    auto topSlice = bounds.removeFromTop (90);
    topSlice.removeFromTop (6);
    titleLabel.setBounds (topSlice.removeFromTop (40));
    versionLabel.setBounds (topSlice.removeFromTop (24));
    copyrightLabel.setBounds (topSlice.removeFromTop (24));
    bounds.removeFromTop (2);
    tabs.setBounds (bounds.reduced (4));
    copyVersionButton.setBounds (
        getWidth() - 44, tabs.getY() - 2, 36, 20);
}

void AboutScreen::paint (Graphics& g)
{
    g.fillAll (findColour (DocumentWindow::backgroundColourId));

    if (elementLogo != nullptr)
        elementLogo->drawWithin (g, elementLogoBounds, RectanglePlacement::centred, 1.0);
    else
    {
        logo.drawWithin (g, elementLogoBounds, RectanglePlacement::centred, 1.0);
    }
}

void AboutScreen::updateAboutInfo()
{
    const auto buildDate = Time::getCompilationDate();

    if (info.title.isNotEmpty())
    {
        titleLabel.setText (info.title, sendNotification);
    }

    if (info.version.isNotEmpty())
    {
        String version = info.version;
        version << "\nBuild date: " << String (buildDate.getDayOfMonth()) << " " + Time::getMonthName (buildDate.getMonth(), true) << " " + String (buildDate.getYear());

        versionLabel.setText (version, sendNotification);
    }

    if (info.copyright.isNotEmpty())
    {
        copyrightLabel.setText (info.copyright, sendNotification);
    }

    if (! info.link.isEmpty())
    {
        aboutButton.setURL (info.link);
    }

    if (! info.linkText.isNotEmpty())
    {
        aboutButton.setButtonText (info.linkText);
    }

    if (info.licenseText.isNotEmpty())
    {
        const auto idx = tabs.getTabNames().indexOf ("License");
        if (auto t = dynamic_cast<LicenseTextComponent*> (tabs.getTabContentComponent (idx)))
        {
            t->setLicenseText (info.licenseText);
            tabs.setCurrentTabIndex (0);
        }
    }

    if (info.logo.isValid())
    {
        elementLogo.reset();
        logo.setImage (info.logo);
    }

    resized();
    repaint();
}

void AboutScreen::copyVersion()
{
    String txt = info.title;
    txt << juce::newLine << "-------" << juce::newLine << info.version << juce::newLine << info.copyright;
    juce::SystemClipboard::copyTextToClipboard (txt);
}

} // namespace element
