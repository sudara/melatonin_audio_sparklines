# To add AudioBlock summaries to your lldb debugger on MacOS
# Put this line in a file named ~/.lldbinit (create it if necessary)
#     command script import ~/path/to/melatonin_audio_sparklines/sparklines.py
# 
#
# To modify this script, you can manually import / reload as needed from the lldb CLI 
#     command script import ~/projects/melatonin_audio_sparklines/sparklines.py
#
# When loaded, you will see the following in your debugger output:
#     loading melatonin audio sparklines for JUCE AudioBlocks...

# Variable formatting docs https://lldb.llvm.org/use/variable.html

import sys
import lldb

def __lldb_init_module(debugger, dict):
    print("loading melatonin audio sparklines for JUCE AudioBlocks...")
    debugger.HandleCommand('type synthetic add -x "juce::dsp::AudioBlock<" --python-class sparklines.AudioBlockChannelsProvider -w juce')
    debugger.HandleCommand('type summary add -x "juce::dsp::AudioBlock<" -F sparklines.audio_block_summary -w juce')

def audio_block_summary (valobj,internal_dict):
    num_channels = valobj.GetChildMemberWithName('numChannels').GetValueAsUnsigned()
    num_samples = valobj.GetChildMemberWithName('numSamples').GetValueAsUnsigned()
    start_sample = valobj.GetChildMemberWithName('startSample').GetValueAsUnsigned() # private offset that subblocks have
    if num_channels is None or num_channels > 16:
        return "uninitilized AudioBlock"
    
    minValue = 0
    maxValue = 0
    
    # At this point, the AudioBlockChannelsProvider has already intialized
    # That provider replaces all childen, so "channels" is no longer present
    # but we have channel[0] and friends
    for channel_index in range (0, num_channels):
        # We are listing the channels after the sparkline
        channel = valobj.GetChildAtIndex(channel_index + num_channels)
        for sample_index in range(0, num_samples):
            sample = float(channel.GetChildAtIndex(sample_index).GetValue())
            if sample > maxValue:
                maxValue = sample
            if sample < minValue:
                minValue = sample
        
    return str(num_channels) + ' channel(s), ' + str(num_samples) + ' samples, ' + 'min ' + str(minValue) + ', max ' + str(maxValue) 

# Called before the provider is setup
# So the channels child still exists and hasn't been replaced by the synthetic
def get_channels_from_audio_block(channels_child_member, num_channels, num_samples):
    channels = []
    first_channel = channels_child_member.GetChildAtIndex(0)
    float_type = first_channel.GetType().GetPointeeType() # float or double
    float_size = float_type.GetByteSize()
    
    # channels* is a pointer to float pointers
    # but I'm not sure why there are 16 bytes of offset to start
    offset = first_channel.GetType().GetByteSize() * 2

    for i in range(num_channels):
        channels.append(channels_child_member.CreateChildAtOffset('channel[' + str(i) + ']', offset, float_type.GetArrayType(num_samples)))
        offset += num_samples * float_size
    return channels
    
# Not the friendliest way to dig into a float pointer, but it works
# https://stackoverflow.com/a/41134167
def get_samples_from_channel(channel, start_sample, num_samples):
    sample_list = []
    
    # Appease lldb
    float_type = channel.GetChildAtIndex(0).GetType() # float or double
    float_size = float_type.GetByteSize()
    target = lldb.debugger.GetSelectedTarget()
    
    # Grab the pointer location, take into consideration the AudioBlock startSample
    offset = start_sample * float_size
    #print("channel: " + str(channel))
    #print("offset: " + str(offset))
    for sample in range (0, num_samples):
        # GetValueAsUnsigned() outputs this as 0, so lets cast to float instead
        value = float(channel.CreateChildAtOffset('channel[' + str(sample) + ']', offset, float_type).GetValue())
        sample_list.append(value)
        offset += float_size
    return sample_list

def sparkline(samples):
    waveform = "_⎽⎼—⎻⎺‾"
    maxValue = max(samples)
    if (maxValue > 0.0):
        scale = maxValue
    else:
        scale = 1.0 
    num_zeros = 0
    output = "["
    for i in range(len(samples)):
        sample = samples[i]
        # print(str(i) + ": " + str(sample))
        if sample == 0.0:
            if(num_zeros == 0):
                output += '0'
            num_zeros += 1
            continue
        else: 
            num_zeros = 0
                
        if num_zeros > 1:
            output += "(" + str(num_zeros) + ")"
            num_zeros = 0
        # for some reason sys.float_info.epsilon is nowhere near
        # the rounding error introduced by lldb when importing float values to python
        if ((abs(sample) - 0.00000015) > 1.0): # out of bounds?
            output += "E"
        elif ((i > 0) and ((sample < 0) is not (samples[i-1] < 0))):
            output += "x" # zero crossing
        else:
            # normalize so we can see detail
            sample /= scale
            # make it a positive number that varies from 0 to 6
            index = int((sample + 1) / 2.0 * 6.99)
            character = waveform[index]
            if output[-1] != character:
                output += character
                
    if num_zeros > 1:
        output += "(" + str(num_zeros) + ")"
           
    output += "]"
    print(output)
    return output

# This lets us create "synthetic children" of an AudiaBlock
# Which lets us display a sparkline per AudioBlock channel
# https://lldb.llvm.org/use/variable.html#synthetic-children
class AudioBlockChannelsProvider:
    def __init__(self, valobj, internalDict):
        self.valobj = valobj
        self.num_channels = self.valobj.GetChildMemberWithName('numChannels').GetValueAsUnsigned()
        print("number of channels: " + str(self.num_channels))
        self.num_samples = valobj.GetChildMemberWithName('numSamples').GetValueAsUnsigned()
        self.start_sample = valobj.GetChildMemberWithName('startSample').GetValueAsUnsigned() # private offset that subblocks have
    
        # channels is a pointer to float pointers, so we need to tease out our actual channels
        self.channels = get_channels_from_audio_block(valobj.GetChildMemberWithName('channels'), self.num_channels, self.num_samples)
        
    def num_children(self): 
        return self.num_channels + 4

    def get_child_index(self, name): 
        if str.startswith(name, "sparkline"):
            return int(name.lstrip('[').rstrip(']'))
        elif str.startswith(name, "channel["):
            return self.num_channels + int(name.lstrip('[').rstrip(']'))
        elif name == 'numChannels':
            return self.num_channels * 2 
        elif name == 'numSamples':
            return self.num_channels * 2 + 1
        elif name == 'startSample':
            return self.num_channels * 2 + 2
        

    def get_child_at_index(self, index):
        if index < self.num_channels:
            return self.sparkline_value(index, get_samples_from_channel(self.channels[index], self.start_sample, self.num_samples))
        elif index < self.num_channels * 2:
            return self.channels[index - self.num_channels]
        elif index == self.num_channels * 2:
            return self.valobj.GetChildMemberWithName('numChannels')
        elif index == self.num_channels * 2 + 1:
            return self.valobj.GetChildMemberWithName('numSamples')
        elif index == self.num_channels * 2 + 2:
            return self.valobj.GetChildMemberWithName('startSample')
            
            
    def update(self): 
        # Cache the children
        return True

    def has_children(self): 
        return self.num_channels > 0
        
    
    def sparkline_value(self, index, samples):
        string = sparkline(samples)
        # SBValue creation for a string took a while to figure out
        # Unfortunately I couldn't just use an lldb expression because of a bug with quotes in Clion:
        # https://youtrack.jetbrains.com/issue/CPP-25517
        # 
        # Once resolved, the below code could be replaced with:
        # return self.valobj.CreateValueFromExpression("sparkline", '(char *) "hello world"') # clion quotes escaping doesn't work 
        #
        # I also tried creating via Address before I realized Array types existed 
        # https://stackoverflow.com/a/64473058
        # 
        # Other examples of SBValue creation:
        # https://github.com/llvm-mirror/lldb/blob/master/examples/synthetic/bitfield/example.py
        # https://dev.to/vejmartin/prettifying-debug-variables-in-c-with-lldb-34lf
        #
        # Let's first "reach through" our root object to access the type system
        # https://lldb.llvm.org/python_reference/lldb.SBData-class.html
        child_type = self.valobj.target.GetBasicType(lldb.eBasicTypeChar)
        byte_order = self.valobj.GetData().GetByteOrder()
        data = lldb.SBData.CreateDataFromCString(byte_order, child_type.GetByteSize(), string)
        
        # Took me a while to figure out GetArrayType even existed!
        # https://invent.kde.org/tcanabrava/kdevelop/blob/5bd8475f3e0893fb8af299ea1f99042c2c2324bd/plugins/lldb/formatters/helpers.py#L279-281
        return self.valobj.CreateValueFromData("sparkline[" + str(index) + "]", data, child_type.GetArrayType(data.GetByteSize()))