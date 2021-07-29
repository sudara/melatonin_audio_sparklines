/*
BEGIN_JUCE_MODULE_DECLARATION

 ID:               melatonin_audio_sparklines
 vendor:           Sudara
 version:          1.0.0
 name:             Melatonin Audio Sparklines
 description:      Tufte would B proud
 license:          MIT
 dependencies:     juce_dsp

END_JUCE_MODULE_DECLARATION
*/

#pragma once
#include <juce_dsp/juce_dsp.h>

#define MELATONIN_SPARKLINE_ENABLED

namespace melatonin
{
    template <typename SampleType>
    using AudioBlock = juce::dsp::AudioBlock<SampleType>;

    template <typename T> struct TypeNameString               { static std::string get() { return typeid (T).name(); } };
    template <typename T> struct TypeNameString<const T>      { static std::string get() { return "const " + TypeNameString<std::remove_const_t<T>>::get(); }};
    template <>           struct TypeNameString<float>        { static std::string get() { return "float"; } };
    template <>           struct TypeNameString<double>       { static std::string get() { return "double"; } };

    template <typename T> std::string typeNameString() { return TypeNameString<T>::get(); }

    // This only reports when more than one 0 is in a row
    // In other words, it won't report a single zero starting a sine wave, for example
    template <typename SampleType>
    static inline size_t numberOfConsecutiveZeros (const AudioBlock<SampleType>& block)
    {
        size_t consecutiveZeros = 0;
        for (int c = 0; c < (int) block.getNumChannels(); ++c)
        {
            for (int i = 0; i < (int) block.getNumSamples(); ++i)
            {
                auto lastSample = i > 0 ? block.getSample (c, i - 1) : 0;
                auto currentSample = block.getSample (c, i);
                if (currentSample == 0 && lastSample == 0)
                    consecutiveZeros++;
            }
        }
        return consecutiveZeros > 1 ? consecutiveZeros : 0;
    }

    template <typename SampleType>
    static inline float percentFilled (const AudioBlock<SampleType>& block)
    {
        size_t numberOfSamples = block.getNumChannels() * block.getNumSamples();
        return (numberOfSamples - numberOfConsecutiveZeros (block)) / (float) numberOfSamples * 100;
    }

    template <typename SampleType>
    static inline juce::String summaryOf (const AudioBlock<SampleType>& block)
    {
        // We need to handle the case where e.g. an AudioBlock<const float> is passed to this function
        using NonConstSampleType = std::remove_const_t<SampleType>;

        std::vector<NonConstSampleType> min (block.getNumChannels());
        std::vector<NonConstSampleType> max (block.getNumChannels());

        for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
        {
            min[ch] = juce::FloatVectorOperations::findMinimum (block.getChannelPointer (ch), (int) block.getNumSamples());
            max[ch] = juce::FloatVectorOperations::findMaximum (block.getChannelPointer (ch), (int) block.getNumSamples());
        }

        const auto overallMin = juce::FloatVectorOperations::findMinimum (min.data(), (int) block.getNumChannels());
        const auto overallMax = juce::FloatVectorOperations::findMaximum (max.data(), (int) block.getNumChannels());

        std::ostringstream summary;

        summary << "AudioBlock<" << typeNameString<SampleType>() <<
                   "> (" << block.getNumChannels() << (block.getNumChannels() == 1 ? " channel, " : " channels, ") <<
                   block.getNumSamples() << " samples, min " <<
                   overallMin << ", max " <<
                   overallMax << ", " << percentFilled (block) << "% filled) with content\n";

        return summary.str();
    }

    /* A healthy waveform might look like this:
       [0—⎻‾⎺‾⎻—x—⎼⎽_⎽⎼—x—⎻‾⎺‾⎻—x—⎼⎽_⎽⎼—]

       0 = true 0
       x = zero crossing
       E = out of bounds (below -1.0 or above 1.0)

     */
    template <typename SampleType>
    static inline juce::String sparkline (const AudioBlock<SampleType>& block, bool collapse = true, bool normalize = true)
    {
        juce::String sparkline = summaryOf (block);
        for (int c = 0; c < (int) block.getNumChannels(); ++c)
        {
            // Xcode and MacOS Terminal font rendering flips the height of ‾ and ⎺
            #ifdef MELATONIN_SPARKLINE_XCODE
                juce::String waveform = juce::CharPointer_UTF8 ("_\xe2\x8e\xbd\xe2\x8e\xbc\xe2\x80\x94\xe2\x8e\xbb\xe2\x80\xbe\xe2\x8e\xba"); // L"_⎽⎼—⎻‾⎺"; // L"˙ॱᐧ-⸱․_"; ˙ॱᐧ-⸱․_ 000  L"▁▂▃▄▅▆▇█";
            #else
                juce::String waveform = juce::CharPointer_UTF8 ("_\xe2\x8e\xbd\xe2\x8e\xbc\xe2\x80\x94\xe2\x8e\xbb\xe2\x8e\xba\xe2\x80\xbe"); // L"_⎽⎼—⎻⎺‾";
            #endif

            float absMax = abs (juce::FloatVectorOperations::findMaximum (block.getChannelPointer (c), (int) block.getNumSamples()));
            float absMin = abs (juce::FloatVectorOperations::findMinimum (block.getChannelPointer (c), (int) block.getNumSamples()));
            float channelMax = juce::jmax (absMin, absMax);

            sparkline += "[";
            size_t numZeros = 0;
            for (int i = 0; i < (int) block.getNumSamples(); ++i)
            {
                float unnormalizedValue = block.getSample (c, i);
                float value = normalize && channelMax != 0 ? unnormalizedValue / channelMax : unnormalizedValue;

                // Asserts when you are dividing by zero...
                // Use printSamples() to figure out what's up
                jassert (!isnan (value));

                juce::juce_wchar output;
                if (value == 0)
                {
                    output = '0';
                    numZeros++;
                }
                else if ((i > 0) && ((value < 0) != (block.getSample (c, i - 1) < 0)))
                    output = 'x';
                else if ((std::abs(unnormalizedValue) - std::numeric_limits<SampleType>::epsilon()) > 1.0 )
                    output = 'E';
                else
                {
                    float normalizedValue = (value + 1) / 2;
                    output = waveform[(int) (normalizedValue * 7.0)];
                }
                if (collapse && (((output != '0') && numZeros > 1) || ((output == '0') && i == (int) (block.getNumSamples() - 1))))
                {
                    sparkline << "(" << numZeros << ")";
                    numZeros = 0;
                }
                else if ((output != '0') && numZeros == 1)
                {
                    sparkline += output;
                    numZeros = 0;
                }
                else if (i == 0 || !collapse || (output != sparkline.getLastCharacter()))
                    sparkline += output;
            }
            sparkline += "]\n";
        }
        return sparkline;
    }

    template <typename SampleType>
    static inline void printSparkline (const AudioBlock<SampleType>& block, bool collapse = true)
    {
        DBG (sparkline (block, collapse));
        juce::ignoreUnused (block, collapse);
    }

    // asArray adds a comma to help you copy and paste the numbers into another context, like python
    template <typename SampleType>
    static inline void printSamples (AudioBlock<SampleType>& block, const int precision = 3, bool asArray = false)
    {
        juce::String output;
        for (int i = 0; i < (int) block.getNumSamples(); i++)
        {
            float value = block.getSample (0, i);
            output += juce::String (value, precision); // 3 significant digits by default
            if (asArray)
                output += ",";
            output += " ";
        }
        DBG (output);
    }
}
