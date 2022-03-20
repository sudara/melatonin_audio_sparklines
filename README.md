Read [my blog article on audio sparklines!](https://melatonin.dev/blog/audio-sparklines/)

# Melatonin Audio Sparklines

This is a C++ JUCE module that summarizes and visualizes what's in an [AudioBlock](https://docs.juce.com/master/classdsp_1_1AudioBlock.html).

It's nice to get a quick idea of what's happening to your JUCE audio buffers during development or tests.

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

If you use CMake (my condolences (jk jk)), inform JUCE about the module in your `CMakeLists.txt`:
```
juce_add_module("modules/melatonin_audio_sparklines")
```

Include the header where needed:

```
#include "melatonin_audio_sparklines/melatonin_audio_sparklines.h"

```

## JUCE Usage: printSparkline

Call `melatonin::printSparkline(myAudioBlock);` and under the hood, DBG will be called. 

If you are lucky, you'll see a healthy looking wave (like this cutie little square wave) spat out into your console:

```
Block is 1 channel, 441 samples, min -0.999994, max 0.999994, 100% filled
[0⎺‾⎺x⎽_⎽]
```

Or a sine: 

```
Block is 1 channel, 441 samples, min -0.999994, max 0.999994, 100% filled
[0—⎻⎺‾⎺⎻—x—⎼⎽_⎽⎼—]
```

You can output the sparkline in full sample-by-sample uncompressed glory with `melatonin::printSparkline(myAudioBlock, false)` or display it without normalization with `melatonin::printSparkline(myAudioBlock, true, false)`.


## lldb installation

Thanks to Jim Credland's [existing lldb formatters](https://github.com/jcredland/juce-toys/blob/master/juce_lldb_xcode.py) I felt bold enough to jump in and figure out how to get sparklines in your lldb-driven IDE.

Put this line in a file named ~/.lldbinit (create it if necessary), pointing to `sparklines.py`:

```
command script import ~/path/to/melatonin_audio_sparklines/sparklines.py
```

On hover you'll get a summary:

![Screenshot 2021-06-22 SineMachine – CycleNote cpp - CLion](https://user-images.githubusercontent.com/472/122944838-6e992a80-d378-11eb-8f87-e7e858da6703.jpg)

On click you'll get sparklines and the ability to browse samples:

![Screenshot 2021-06-22 SineMachine – CycleNote cpp - CLion](https://user-images.githubusercontent.com/472/122945885-2fb7a480-d379-11eb-90c4-2ebe10af1775.jpg)

If you'd like to read more about this, read my post on [creating lldb type summaries and children](https://melatonin.dev/blog/how-to-create-lldb-type-summaries-and-synthetic-children-for-your-custom-types/).

## Feature key

Here are what the symbols mean that will show up in the sparkline

```
0 = true 0
x = zero crossing
E = out of bounds (below -1.0 or above 1.0)
I = Inf (Infinity, meaning you've divided by zero)
N = NaN (undefined calculation that's not INF)
S = Subnormal detected
```

## Caveats

`printSparklines` was tested on VS2019 on Windows, Xcode on MacOS and CLion on Windows and MacOS.

The lldb python script was tested on MacOS CLion and Xcode only.

### Xcode gotcha

If you are using Xcode exclusively, you might want to plop this somewhere

```cpp
#define MELATONIN_SPARKLINE_XCODE=1
```

MacOS native font rendering seems to flip the height of two of the characters (⎺ vs. ‾) but it's fine in MacOS CLion, etc. 

### Don't like how they look?

I'm not a fan of VS2019 with the default Consolas font. Everywhere else it seems to look good! Open an issue if you can make it nicer!

### Reminder: Don't leave a printSparkline call in your audio path

Not only does `DBG` itself allocate, but the whole name of the game is string manipulation here, so if you are sticking a `printSparkline` in your audio callback, expect those sweet sweet dropouts.
