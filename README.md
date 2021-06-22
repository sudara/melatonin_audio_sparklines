# Melatonin Audio Sparklines

This is a C++ JUCE module that summarizes and visualizes what's in an [AudioBlock](https://docs.juce.com/master/classdsp_1_1AudioBlock.html).

It's nice to get a quick idea of what's happening to your audio buffers during development or tests.

## printSparkline

Call `melatonin::printSparkline(myAudioBlock);` and under the hood, DBG will be called. 

If you are lucky you'll see a healthy looking wave, like this cutie little square wave spat out into your console:

```
Block is 1 channel, 441 samples, min -0.999994, max 0.999994, 100% filled
[0⎺‾⎺x⎽_⎽]
```

Or a sine: 

```
Block is 1 channel, 441 samples, min -0.999994, max 0.999994, 100% filled
[0—⎻⎺‾⎺⎻—x—⎼⎽_⎽⎼—]
```

Get the sparkline in its full sample-by-sample verbose glory:

`melatonin::printSparkline(myAudioBlock, false);`


If you don't want the output normalized for some reason:

`melatonin::printSparkline(myAudioBlock, true, false);`

## Teach lldb how to output sparklines

Thanks to Jim Credland's [lldb script](https://github.com/jcredland/juce-toys/blob/master/juce_lldb_xcode.py) I felt bold enough to jump in and figure out how to get sparklines in your lldb-driven IDE.

Put this line in a file named ~/.lldbinit (create it if necessary), pointing to `sparklines.py`:

```
command script import ~/path/to/melatonin_audio_sparklines/sparklines.py
```

On hover you'll get a summary:

![Screenshot 2021-06-22 SineMachine – CycleNote cpp - CLion](https://user-images.githubusercontent.com/472/122944838-6e992a80-d378-11eb-8f87-e7e858da6703.jpg)

On click you'll get sparklines and the ability to browse samples:

![Screenshot 2021-06-22 SineMachine – CycleNote cpp - CLion](https://user-images.githubusercontent.com/472/122945885-2fb7a480-d379-11eb-90c4-2ebe10af1775.jpg)


## Installation 

Set up a git submodule in your project tracking the `main` branch. I usually stick mine in a `modules` directory.

```git
git submodule add -b main https://github.com/sudara/melatonin_audio_sparklines modules/melatonin_audio_sparklines
git commit -m "Added melatonin_audio_sparklines submodule."
```

To update melatonin_audio_sparklines, you can:
```git
git submodule update --remote --merge modules/melatonin_audio_sparklines
```

If you use CMake (my condolences), inform JUCE about the module in your `CMakeLists.txt`:
```
juce_add_module("modules/melatonin_audio_sparklines")
```

Include the header where needed:

```
#include "melatonin_audio_sparklines/melatonin_audio_sparklines.h"

```


## Motivation

I've been working on unit tests at a low level. It's been going well, but I kept feeling like I didn't have enough insight into what my various buffers and blocks of audio look like. 

There's nothing worse than having a test fail and then not really having a way to quickly verify what failed.

I found myself wanting to answer the following, quickly:

* Is there a signal present, and is it sinusoidal?
* How many cycles are there?
* Is all or part of the block empty?
* Are there any values that are out of bounds?

Basically, I wanted a visual waveform. In my UI, I do have a little helper that I can turn on to show me visually what's in my buffer, a modified version of Jim Credland's buffer debugger.

But a waveform isn't possible in the debugging console and I'm not always working in a UI context. It's trivial to write little helpers to report on zero crossings, etc. But in the context of debugging when you don't know exactly what's wrong, having to manually swap helpers in and out and compile again and again is just not friendly.

One can always write a bunch of float values to the screen, but humans can't reliably parse thousands of float values visually. Have fun counting those zero crossings manually!

## Sparklines

I'm a big [Edward Tufte](https://www.edwardtufte.com/tufte/) fan and one of the things he champions is "data-intense, design-simple, word-sized graphics" which he gave the name "[sparkline](https://en.wikipedia.org/wiki/Sparkline)."

At a glance, you can tell some general characteristics about the data series. Individual data points are not relevant, the trend is! 

I figured it might be possible to make an sparkline graph with unicode and saw that not only does unicode have [block elements](https://en.wikipedia.org/wiki/Block_Elements) but there are [already people doing sparkline bar graphs](https://rosettacode.org/wiki/Sparkline_in_unicode). 

However, the block elements are vertically bottom aligned instead of center aligned, meaning they don't really parse easily as a waveform.

## Audio Sparklines

In the end, I chose the following 7 symbols to represent the waveform. Yup, we're decimating the audio down to 3 bits.

```
_⎽⎼—⎻⎺‾
```

In addition to the following features:

```
0 = true 0
x = zero crossing
E = out of bounds (below -1.0 or above 1.0)
```

Here's what 2 cycles of a healthy sine wave look like (it keeps scrolling right) with all samples represented:


```
[0———⎻⎻⎻⎻⎻⎻⎻‾‾‾‾‾‾‾‾⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺‾‾‾‾‾‾‾‾‾⎻⎻⎻⎻⎻⎻⎻———x——⎼⎼⎼⎼⎼⎼⎼⎽⎽⎽⎽⎽⎽⎽⎽⎽____________________________________⎽⎽⎽⎽⎽⎽⎽⎽⎼⎼⎼⎼⎼⎼⎼————x——⎻⎻⎻⎻⎻⎻⎻‾‾‾‾‾‾‾‾⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺⎺‾‾‾‾‾‾‾‾‾⎻⎻⎻⎻⎻⎻⎻———x——⎼⎼⎼⎼⎼⎼⎼⎽⎽⎽⎽⎽⎽⎽⎽⎽____________________________________⎽⎽⎽⎽⎽⎽⎽⎽⎼⎼⎼⎼⎼⎼⎼———]
```

Cool! It's bulky and elongated, but we can see some trends...

## Cleaning them up 

To do Tufte right, we should increase the amount of signal per surface area. We can collapse the redundant data, leaving us with just the "trend."

```
[0⎻⎺‾⎺⎻x—⎼⎽_⎽⎼—x⎻⎺‾⎺⎻x—⎼⎽_⎽⎼—] 294 samples (-0.999 to 0.999)
```


This is useful. We can glean a lot of info from this example:

* We can quickly visually identify 2 healthy full amplitude cycles. 
* We know the first value is zero.
* It looks sinusoidal
* We know there are 294 samples in total
* We know the amplitude ranges from -0.999 to 0.999.

### Normalized

Without normalization, quiet volumes might get something that looks like this:

```
Block is 2 channels, 128 samples, min -0.0951679, max 0.11609, 50.7812% filled
[—x—0(64)]
```

Which is....pretty cryptic. Nothing a bit of normalization can't help with, though:

```
Block is 2 channels, 128 samples, min -0.0951679, max 0.11609, 50.7812% filled
[‾⎺⎻—x—⎼⎽_0(64)]
```

Now we can see the shape of the waveform and look to the metadata for the scale.

## More complex examples

Here's an example of block going out of audio bounds. The `E` lets you know a sample is out of bounds. We can see a big empty chunk of zeros at the end of the block, and the number 234 tells us exactly how many consecutive zeros there are.

```
[0⎻⎺⎻x—x⎻x—⎼⎽_E_⎽⎼—x⎻⎺‾E‾⎺⎻x—⎼—x⎻⎺⎻x—⎼⎽_E_⎽⎼—0⎻⎺‾E‾⎺⎻x—⎼—x⎻⎺⎻x—⎼⎽_E_⎽⎼—0(234)] 1024 samples (-1.76012, 1.76013)

```
 
The collapsed version looks more clearly sinusoidal, but we can see it's going out of bounds. We still have a precise grasp of how many samples in the buffer are empty.


## Caveats

Tested on VS2019 on Windows, Xcode on MacOS and CLion on Windows and MacOS.

### Xcode gotcha

If you are using Xcode exclusively, you might want to plop this somewhere

```cpp
#define MELATONIN_SPARKLINE_XCODE=1
```

MacOS font rendering flips the height of ⎺ and ‾, but it's fine in MacOS CLion, etc. 

### Don't like how they look?

Yeah, I'm not a fan of VS2019 with the default Consolas font. Everywhere else it seems to look good! Open an issue if you can make it nicer!

### Reminder: Don't leave printSparkline in your audio path

Not only does `DBG` itself allocate, but the whole name of the game is string manipulation here, so if you are sticking a `printSparkline` in your audio callback, expect those sweet sweet dropouts.


