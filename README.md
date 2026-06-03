# react-native-openwakeword

Offline wake word detection for React Native, powered by TensorFlow Lite openWakeWord models and a native Nitro JSI bridge.

[![CI](https://github.com/incognitol07/react-native-openwakeword/actions/workflows/ci.yml/badge.svg)](https://github.com/incognitol07/react-native-openwakeword/actions/workflows/ci.yml)
[![Version](https://img.shields.io/npm/v/react-native-openwakeword.svg)](https://www.npmjs.com/package/react-native-openwakeword)
[![Platform](https://img.shields.io/badge/platform-ios%20%7C%20android-blue.svg)](https://www.npmjs.com/package/react-native-openwakeword)
[![Downloads](https://img.shields.io/npm/dm/react-native-openwakeword.svg)](https://www.npmjs.com/package/react-native-openwakeword)
[![License](https://img.shields.io/npm/l/react-native-openwakeword.svg)](https://github.com/Incognitol07/react-native-openwakeword/blob/master/LICENSE)

`react-native-openwakeword` is the inference engine. It does not record audio for you. Bring your own microphone/audio library, feed the engine 16 kHz mono `int16` PCM buffers, and it returns wake word probabilities synchronously from native code.

## Features

- Offline wake word inference with TensorFlow Lite models.
- Nitro JSI native bridge for low-overhead synchronous calls.
- Android and iOS support.
- Streaming C++ pipeline with fixed ring buffers for audio, mel frames, and embedding frames.
- Native reset and threshold controls.
- Optional Android native perf logs for profiling your own device.

## Requirements

- React Native `0.74.0` or newer
- `react-native-nitro-modules`
- Android API `24` or newer
- iOS `15.1` or newer
- Node `20.0.0` or newer

## Installation

```sh
npm install react-native-openwakeword react-native-nitro-modules
```

Run platform setup for your app as usual.

```sh
cd ios
pod install
```

## Models

The detector expects three TensorFlow Lite model files:

1. `melspectrogram.tflite`
2. `embedding_model.tflite`
3. A wake word model, for example `hey_jarvis.tflite`

The models must be available as real files on the device filesystem. On Android, do not pass a bundled asset URI directly to `loadModels()`. Copy or download the models into app storage first, then pass absolute paths.

## Usage

```ts
import { Openwakeword } from 'react-native-openwakeword'

const loaded = Openwakeword.loadModels(
  '/absolute/path/to/melspectrogram.tflite',
  '/absolute/path/to/embedding_model.tflite',
  '/absolute/path/to/hey_jarvis.tflite'
)

if (!loaded) {
  throw new Error('Failed to load wake word models')
}

Openwakeword.setThreshold(0.5)
```

Feed 16 kHz, mono, signed 16-bit PCM audio into `processFrame()`:

```ts
function onAudioBuffer(buffer: ArrayBuffer) {
  const result = Openwakeword.processFrame(buffer)

  if (result.isDetected) {
    console.log('Wake word detected', result.probability)
  }
}
```

If you restart a voice session or want to discard previous streaming context, reset the internal buffers:

```ts
Openwakeword.reset()
```

## Audio Input

Use an audio library that can deliver raw PCM buffers. Configure it for:

| Setting | Value |
| --- | --- |
| Sample rate | `16000` Hz |
| Channels | `1` |
| Format | signed 16-bit PCM |
| JS type | `ArrayBuffer` |

The native engine processes audio in 1280-sample windows, which is about 80 ms at 16 kHz.

## API

### `loadModels(melspecPath, embeddingPath, wakeWordPath): boolean`

Loads the three `.tflite` models and prepares TensorFlow Lite interpreters.

```ts
const ok = Openwakeword.loadModels(melspecPath, embeddingPath, wakeWordPath)
```

### `processFrame(buffer): DetectionResult`

Processes incoming PCM audio and returns the latest wake word score.

```ts
type DetectionResult = {
  probability: number
  isDetected: boolean
}
```

### `setThreshold(threshold): void`

Sets the probability threshold used for `isDetected`.

```ts
Openwakeword.setThreshold(0.5)
```

### `reset(): void`

Clears streaming audio, mel, and embedding buffers.

```ts
Openwakeword.reset()
```

## Performance

The Android example app was profiled on a physical Samsung SM-X115 tablet with a MediaTek MT8781V/CA SoC running Android 16. Measurements were collected with native timing instrumentation, `adb shell top`, and `adb shell dumpsys meminfo`.

| Metric | Observed |
| --- | ---: |
| Audio window size | 1280 samples |
| Audio duration per window | ~80 ms at 16 kHz |
| Average native `processFrame` time | ~16-21 ms |
| Maximum observed `processFrame` time | ~31-46 ms |
| Average melspec model time | ~3.3-5.2 ms |
| Average embedding model time | ~11.4-15.5 ms |
| Average wakeword model time | ~0.7-1.7 ms |
| Steady-state app CPU while listening | ~5-7% |
| App total PSS memory | ~101 MB |
| Native heap PSS | ~32 MB |

In this test, the native pipeline stayed comfortably below the ~80 ms real-time audio window budget. The embedding model was the dominant inference cost.

Performance will vary by device, model files, audio library, app workload, React Native version, and build type. Always test on physical devices before shipping an always-listening experience.

## Profiling

Android perf logging can be enabled with a system property:

```sh
adb shell setprop debug.openwakeword.perf 1
adb logcat -s OpenWakeWord
```

The native module logs aggregated timing every few seconds while audio is being processed:

```text
OpenWakeWord: perf calls=63 windows=63 avg_total=18.392ms max_total=35.179ms avg_mel=4.371ms max_mel=10.954ms avg_embedding=12.589ms max_embedding=24.917ms avg_wakeword=1.413ms max_wakeword=6.194ms
```

Disable logs with:

```sh
adb shell setprop debug.openwakeword.perf 0
```

Useful device checks:

```sh
adb shell top -b -d 1 -n 60 -p <pid>
adb shell dumpsys meminfo <your.package.name>
```

## Notes

- The library currently expects model paths, not bundled asset IDs.
- Debug builds are not representative of release performance.

## Example

The `example/` directory contains a React Native app for loading models, streaming microphone audio, and testing detection behavior on a device.

```sh
cd example
npm run android
```

## Credits

Built with [Nitro Modules](https://nitro.margelo.com/) and powered by [openWakeWord](https://github.com/dscripka/openWakeWord) model architecture.

Bootstrapped with [create-nitro-module](https://github.com/patrickkabwe/create-nitro-module).

## Contributing

Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

## License

Apache-2.0
