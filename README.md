# Audio Buffers Library (ABL)

A small c++ library that implement normal, circular and delayed circular single channel and multi channels audio buffers.

The buffers are offered in two format: 
- A view format that permit to read and modify a contiguous area in memory (single channel) or more contiguous area in memory (multi channels) 
- An owning format that extend the view format allocating and managing internally a contiguous memory area 

The various buffers offer a similar API with function to get/set/add sample, copy/add sample from another buffer with a gain or a gain ramp, apply a gain/gain ramp, clear and reverse the data.
The buffers can be iterated: iterating a single channel buffer return the samples and iterating a multi channels buffer return the single channel buffer for channel.
All the buffers are templated and can accept a Numeric type (integral or floating point).

The library is written using c++20 features and is currently a work in progress. All the main function are probably there, but I'm still experimenting, so the API could change.
There are many "@todo" in the codes for the parts I'm still working on or I'm searching for solutions. (Any help or suggestion would be appreciated!)

It was initially written using some common interfaces for all the types but because of some performance issue caused by the allocation/deallocation required to have polymorphic types was partially converted to using c++20 concepts and boost::variant2 to optimize performance (I'm not fully convinced by this change, because it makes you use variant and concepts to do type erasure and it's still not a confortable developer experience as today compared to Interfaces and polymorphism).

The library have a good coverage done with unit test written in catch2, and also offer some small benchmark to compare the use of the library as iterable with the raw pointers iteration.

## Buffer types
There are manly 2 types of buffers is this library:
- Normal: Normal audio buffer
- Circular: Buffer that contains more that one singular buffers and permit to have one of those single buffer between the end and the start of the memory space, shifting the indexes to the real memory position

Here's a list of all the buffers:
### Single channel buffer view
- **AudioBufferChannelView**: Normal audio buffer view
- **CircularAudioBufferChannelView**: Circular audio buffer view
- **OffsettedReadCircularAudioBufferChannelView**: Circular audio buffer view with offsetted read (used when read index is different from write index)
- **AudioBufferChannelViewWrapper**: Variant type that can hold a AudioBufferChannelView, CircularAudioBufferChannelView or OffsettedReadCircularAudioBufferChannelView and permit to use all their common functions
- **AudioBufferChannelViewConcepts**: Contains the AudioBufferChannelReadableType and AudioBufferChannelType concepts that can be used to accept generically all the buffer channel views as a function parameter 

### Multi channels buffer view
- **AudioBufferView**: Normal audio buffer
- **CircularAudioBufferView**: Circular audio buffer view with an internal read and a write indexes
- **DelayedCircularAudioBufferView**: Circular audio buffer view with an internal write index and a virtual read index that sum a delay to the write position
- **AudioBufferViewWrapper**: Variant type that can hold a AudioBufferView, CircularAudioBufferView or DelayedCircularAudioBufferView and permit to use all their common functions
- **AudioBufferViewConcepts**: Contains the AudioBufferReadableType, AudioBufferType, CircularAudioBufferReadableType, CircularAudioBufferType, DelayedCircularAudioBufferReadableType and DelayedCircularAudioBufferType concepts that can be used to accept generically all the buffer channel views as a function parameter

### Multi channels buffer
- **AudioBuffer**: AudioBufferView that manage internally his memory and can clone an existing buffer
- **CircularAudioBuffer**: CircularAudioBufferView that manage internally his memory and can clone an existing buffer
- **DelayedCircularAudioBuffer**: DelayedCircularAudioBufferView that manage internally his memory and can clone an existing buffer

## Examples

Applying a gain ramp and iterating an existing memory using multi channels view
```
size_t channels = 2;
size_t bufferSize = 8;
auto data = new double*[channels];
for (size_t channel = 0; channel < channels; ++channel) {
    data[channel] = new double[bufferSize];
    std::fill(data[channel], data[channel] + bufferSize, 10.0);
}

abl::AudioBufferView<double> audioBufferView{data, channels, bufferSize};
audioBufferView.applyGainRamp(0.0, 1.0);
for (auto&& channelView: audioBufferView) {
    for (auto &&sample: channelView) {
        //doSomethingWith(sample);
    }
}
```

Create a delayed circular buffer of size 32, with single buffers of size 8 and a delay of 8. Clear the buffer, add 20 to all the samples and read it after
```
abl::DelayedCircularAudioBuffer<int> delayedAudioBuffer{32, 8, 8};
delayedAudioBuffer.clear();
for (auto&& channelView: delayedAudioBuffer) {
    for(size_t index = 0; index < channelView.getBufferSize(); ++index) {
        channelView.addSample(index, 20 + index);
    }
}

delayedAudioBuffer.incrementIndex();
for (auto&& channelView: delayedAudioBuffer) {
    for (auto &&sample: channelView) {
        //doSomethingWith(sample);
    }
}
```