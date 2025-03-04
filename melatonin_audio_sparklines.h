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
        return (float) (numberOfSamples - numberOfConsecutiveZeros (block)) / (float) numberOfSamples * 100;
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

        summary << "Block is " << block.getNumChannels() << " channels, " << block.getNumSamples() << " samples, min " << overallMin << ", max " << overallMax << ", " << percentFilled (block) << "% filled\n";
        return summary.str();
    }

    /* A healthy waveform might look like this:
       [0—⎻‾⎺‾⎻—x—⎼⎽_⎽⎼—x—⎻‾⎺‾⎻—x—⎼⎽_⎽⎼—]

       0 = true 0
       x = zero crossing
       E = out of bounds (below -1.0 or above 1.0)

     */
    template <typename SampleType>
    [[nodiscard]] static inline juce::String sparkline (const SampleType* data, size_t numSamples, bool collapse = true, bool normalize = true)
    {
        juce::String sparkline;
// Xcode and MacOS Terminal font rendering flips the height of ‾ and ⎺
#ifdef MELATONIN_SPARKLINE_XCODE
        juce::String waveform = juce::CharPointer_UTF8 ("_\xe2\x8e\xbd\xe2\x8e\xbc\xe2\x80\x94\xe2\x8e\xbb\xe2\x80\xbe\xe2\x8e\xba"); // L"_⎽⎼—⎻‾⎺"; // L"˙ॱᐧ-⸱․_"; ˙ॱᐧ-⸱․_ 000  L"▁▂▃▄▅▆▇█";
#else
        juce::String waveform = juce::CharPointer_UTF8 ("_\xe2\x8e\xbd\xe2\x8e\xbc\xe2\x80\x94\xe2\x8e\xbb\xe2\x8e\xba\xe2\x80\xbe"); // L"_⎽⎼—⎻⎺‾";
#endif

        SampleType absMax = abs (juce::FloatVectorOperations::findMaximum (data, (int) numSamples));
        SampleType absMin = abs (juce::FloatVectorOperations::findMinimum (data, (int) numSamples));
        SampleType channelMax = juce::jmax (absMin, absMax);

        sparkline += "[";
        size_t numZeros = 0;
        for (int i = 0; i < (int) numSamples; ++i)
        {
            SampleType unnormalizedValue = data[i];
            SampleType value = normalize && channelMax != 0 ? unnormalizedValue / channelMax : unnormalizedValue;

            juce::juce_wchar output;
            auto type = std::fpclassify (value);

            if (value == 0)
            {
                output = '0';
                numZeros++;
            }
            else if ((i > 0) && ((value < 0) != (data[i - 1] < 0)))
                output = 'x';
            else if (type == FP_NAN)
                output = 'N';
            else if (type == FP_INFINITE)
                output = 'I';
            else if (type == FP_SUBNORMAL)
                output = 'S';
            else if ((std::abs (unnormalizedValue) - std::numeric_limits<SampleType>::epsilon()) > 1.0)
                output = 'E';
            else
            {
                SampleType normalizedValue = (value + 1) / 2;
                output = waveform[(int) (normalizedValue * 7.0)];
            }
            if (collapse && (((output != '0') && numZeros > 1) || ((output == '0') && i == (int) (numSamples - 1))))
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
        return sparkline;
    }

    template <typename SampleType>
    static inline juce::String sparkline (const AudioBlock<SampleType>& block, bool collapse = true, bool normalize = true)
    {
        juce::String sparkline = summaryOf (block);
        for (size_t c = 0; c < block.getNumChannels(); ++c)
        {
            sparkline += ::melatonin::sparkline<SampleType> (block.getChannelPointer (c), block.getNumSamples(), collapse, normalize);
            sparkline += "]\n";
        }
        return sparkline;
    }

    template <typename SampleType>
    static inline juce::String sparkline (juce::AudioBuffer<SampleType>& buffer, bool collapse = true, bool normalize = true)
    {
        auto block = AudioBlock<SampleType> (buffer);
        return sparkline (block, collapse, normalize);
    }

    template <typename SampleType>
    static inline void printSparkline (const AudioBlock<SampleType>& block, bool collapse = true)
    {
        DBG (sparkline (block, collapse));
        juce::ignoreUnused (block, collapse);
    }

    template <typename SampleType>
    static inline void printSparkline (const SampleType* data, size_t size, bool collapse = true)
    {
        DBG (sparkline<SampleType> (data, size, collapse));
    }

    template <typename SampleType>
    static inline void printSparkline (juce::AudioBuffer<SampleType>& buffer, bool collapse = true)
    {
        DBG (sparkline (buffer, collapse));
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

    template <typename SampleType>
    static inline void printSamples (AudioBlock<SampleType>&& block, const int precision = 3, bool asArray = false)
    {
        printSamples (block, precision, asArray);
    }

    template <typename SampleType>
    static inline void printSamples (juce::AudioBuffer<SampleType>& buffer, const int precision = 3, bool asArray = false)
    {
        printSamples (AudioBlock<SampleType> (buffer), precision, asArray);
    }

    template <typename SampleType>
    static inline void printSamples (std::vector<SampleType> data, const int precision = 3, bool asArray = false)
    {
        juce::String output;
        for (int i = 0; i < (int) data.size(); i++)
        {
            float value = data[i];
            output += juce::String (value, precision); // 3 significant digits by default
            if (asArray)
                output += ",";
            output += " ";
        }
        DBG (output);
    }

    template <typename SampleType>
    static inline void printSamples (SampleType* data, size_t numSamples, const int precision = 3, bool asArray = false)
    {
        juce::String output;
        for (int i = 0; i < (int) numSamples; i++)
        {
            float value = data[i];
            output += juce::String (value, precision); // 3 significant digits by default
            if (asArray)
                output += ",";
            output += " ";
        }
        DBG (output);
    }

    static std::string blockToString (const AudioBlock<float>& block, int decimalPlaces = 6)
    {
        auto output = std::string();

        std::ostringstream oss;
        oss << std::fixed << std::setprecision (decimalPlaces);

        for (size_t i = 0; i < block.getNumSamples(); ++i)
        {
            if (i > 0)
                oss << ", ";
            oss << block.getSample (0, i) << "f";
        }

        return oss.str();
    }

    static std::string vectorToString (const std::vector<float>& v, int decimalPlaces = 6)
    {
        if (v.empty())
            return "";

        std::ostringstream oss;
        oss << std::fixed << std::setprecision (decimalPlaces);

        for (size_t i = 0; i < v.size(); ++i)
        {
            if (i > 0)
                oss << ", ";
            oss << v[i] << "f";
        }

        return oss.str();
    }

}
