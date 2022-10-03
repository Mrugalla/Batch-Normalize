#pragma once

#include <JuceHeader.h>

struct MainComponent  :
    public juce::Component,
    public juce::FileDragAndDropTarget
{
    using Graphics = juce::Graphics;
    using Colour = juce::Colour;
    using Image = juce::Image;
    using DnDSrc = juce::DragAndDropTarget::SourceDetails;
    using StringArray = juce::StringArray;
    using String = juce::String;
    using ImageFileFormat = juce::ImageFileFormat;
    using FileChooser = juce::FileChooser;
    using File = juce::File;
    using uint8 = juce::uint8;
    using FileOutputStream = juce::FileOutputStream;
    using PNGImageFormat = juce::PNGImageFormat;
    using Comp = juce::Component;
    using DragAndDropContainer = juce::DragAndDropContainer;
    using FileDragAndDropTarget = juce::FileDragAndDropTarget;
    using Mouse = juce::MouseEvent;
    using AudioBuffer = juce::AudioBuffer<float>;
    using WavAudioFormat = juce::WavAudioFormat;
    using AudioFormatWriter = juce::AudioFormatWriter;
    using AudioFormatManager = juce::AudioFormatManager;
    using FileOutputStream = juce::FileOutputStream;
    using Decibels = juce::Decibels;

    MainComponent() :
        FileDragAndDropTarget(),
        text("drop folder of\naudio files here"),
        tracks(),
        trackTitles()
    {
        setSize(600, 400);
    }

    String text;
    std::vector<AudioBuffer> tracks;
    std::vector<String> trackTitles;
    std::vector<double> sampleRates;

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black);
        g.setColour(juce::Colours::white);
        g.setFont(24.f);
		g.drawFittedText(text, getLocalBounds(), juce::Justification::centred, 1);
    }

    bool isInterestedInFileDrag(const StringArray& files) override
    {
        const auto numFiles = files.size();
        for (auto f = 0; f < numFiles; ++f)
        {
            auto& file = files[f];
            if (!isAudioFile(file))
                return false;
        }
        return true;
    }

    bool isAudioFile(const String& fileName)
    {
		auto ext = fileName.fromLastOccurrenceOf(".", false, false);
		return ext == "flac" || ext == "wav" || ext == "mp3" || ext == "aiff";
    }

    void filesDropped(const StringArray& files, int x, int y) override
    {
        const auto numTracks = files.size();
		tracks.resize(numTracks);
        trackTitles.resize(numTracks);
        sampleRates.resize(numTracks);
		
		text = "Num Tracks: " + String(numTracks) + "\n";
        
        auto minPeak = 1.f;
        auto maxPeak = 0.f;

        for (auto i = 0; i < numTracks; ++i)
        {
            auto& track = tracks[i];
            auto& trackTitle = trackTitles[i];
            auto& Fs = sampleRates[i];
            const auto& file = files[i];
            loadAudioFile(file, track, Fs);

            auto seperatorString = File::getSeparatorString();
			trackTitle = file.fromLastOccurrenceOf(seperatorString, false, false);
            trackTitle = trackTitle.upToLastOccurrenceOf(".", false, false);

			auto numChannels = track.getNumChannels();
			auto numSamples = track.getNumSamples();

			auto peak = track.getMagnitude(0, numSamples);
            for (auto ch = 1; ch < numChannels; ++ch)
            {
				const auto chPeak = track.getMagnitude(0, numSamples);
				if (chPeak > peak)
					peak = chPeak;
            }

			if (peak < minPeak)
				minPeak = peak;
			if (peak > maxPeak)
				maxPeak = peak;
        }
		
        text += "Min Peak At: " + String(minPeak) + " :: " + String(Decibels::gainToDecibels(minPeak)) + "db\n";
		text += "Max Peak At: " + String(maxPeak) + " :: " + String(Decibels::gainToDecibels(maxPeak)) + "db\n";
        
        const auto gain = Decibels::decibelsToGain(-1.f) / maxPeak;

		text += "Added Gain: " + String(gain) + " :: " + String(Decibels::gainToDecibels(gain)) + "db\n";

        for (auto i = 0; i < numTracks; ++i)
        {
            auto& track = tracks[i];
            auto& trackTitle = trackTitles[i];
            auto& Fs = sampleRates[i];

            auto numChannels = track.getNumChannels();
            auto numSamples = track.getNumSamples();

			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto data = track.getWritePointer(ch);
				for (auto s = 0; s < numSamples; ++s)
					data[s] *= gain;
			}

            saveWav(track, trackTitle, Fs);
        }
		
        repaint();
    }
	
    void loadAudioFile(const File& file, AudioBuffer& buffer, double& sampleRate)
    {
		auto formatManager = std::make_unique<AudioFormatManager>();
		formatManager->registerBasicFormats();
		auto reader = formatManager->createReaderFor(file);
		if (reader != nullptr)
		{
			auto numSamples = reader->lengthInSamples;
			auto numChannels = reader->numChannels;
			sampleRate = reader->sampleRate;
			buffer.setSize(numChannels, numSamples);
			reader->read(&buffer, 0, numSamples, 0, true, true);
		}
        delete reader;
    }

    void saveWav(const AudioBuffer& buffer, const String& title, double Fs)
    {
        const auto path = juce::File::getSpecialLocation
        (
            File::SpecialLocationType::userDesktopDirectory
        ).getFullPathName() + "\\BatchNormalizeOutput\\";
        File file(path);
        file.createDirectory();
        const auto fullName = title + ".wav";
        file = File(path + "\\" + fullName);
        if (file.existsAsFile())
            file.deleteFile();
        WavAudioFormat format;
        std::unique_ptr<AudioFormatWriter> writer;
        const auto numChannels = buffer.getNumChannels();
        writer.reset(format.createWriterFor(new FileOutputStream(file),
            Fs,
            numChannels,
            32,
            {},
            0
        ));
        const auto numSamples = buffer.getNumSamples();
        if (writer != nullptr)
            writer->writeFromAudioSampleBuffer(buffer, 0, numSamples);
    }
};
