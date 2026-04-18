#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>
#include <iostream>
#include <unistd.h>

#define SAMPLE_RATE 8000

static AudioQueueRef queue = nullptr;
static bool done = false;

void audioCallback(void* userData, AudioQueueRef inAQ, AudioQueueBufferRef inBuffer) {
    static int t = 0;
    int16_t* data = (int16_t*)inBuffer->mAudioData;
    const int numSamples = inBuffer->mAudioDataBytesCapacity / 2;

    for (int i = 0; i < numSamples; ++i) {
        uint8_t byte = (uint8_t)(
            (9 * t & (t >> 4)) |
            (5 * t & (t >> 7)) |
            (3 * t & (t >> 10))
        );
        data[i] = (int16_t)(byte - 128) * 256;
        t++;
    }

    inBuffer->mAudioDataByteSize = numSamples * 2;
    AudioQueueEnqueueBuffer(inAQ, inBuffer, 0, nullptr);

    // Stop after ~30 seconds
    if (t > SAMPLE_RATE * 30) {
        done = true;
    }
}

int main() {
    AudioStreamBasicDescription format = {};
    format.mSampleRate       = SAMPLE_RATE;
    format.mFormatID         = kAudioFormatLinearPCM;
    format.mFormatFlags      = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    format.mBitsPerChannel   = 16;
    format.mChannelsPerFrame = 1;
    format.mBytesPerFrame    = 2;
    format.mFramesPerPacket  = 1;
    format.mBytesPerPacket   = 2;

    // Create queue with callback
    OSStatus err = AudioQueueNewOutput(&format, audioCallback, nullptr,
                                       CFRunLoopGetCurrent(), kCFRunLoopCommonModes,
                                       0, &queue);
    if (err != noErr) {
        std::cerr << "AudioQueueNewOutput failed: " << err << std::endl;
        return 1;
    }

    // Allocate two buffers for smooth playback
    AudioQueueBufferRef buffers[2] = {nullptr, nullptr};
    for (int i = 0; i < 2; ++i) {
        err = AudioQueueAllocateBuffer(queue, 4096, &buffers[i]);
        if (err == noErr) {
            audioCallback(nullptr, queue, buffers[i]);  // Fill initially
        }
    }

    err = AudioQueueStart(queue, nullptr);
    if (err != noErr) {
        std::cerr << "AudioQueueStart failed: " << err << std::endl;
        return 1;
    }

    std::cout << "🎵 Playing bytebeat... (Press Ctrl+C to stop)\n";

    // Run the run loop until we're done
    while (!done) {
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.1, true);
    }

    AudioQueueStop(queue, true);
    AudioQueueDispose(queue, true);
    std::cout << "Done.\n";

    return 0;
}