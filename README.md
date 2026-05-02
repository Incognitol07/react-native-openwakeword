# react-native-openwakeword

High-performance, **offline wake word detection** for React Native. Powered by the official [openWakeWord](https://github.com/dscripka/openWakeWord) models and built with the ultra-fast **Nitro JSI** bridge.

[![CI](https://github.com/incognitol07/react-native-openwakeword/actions/workflows/ci.yml/badge.svg)](https://github.com/incognitol07/react-native-openwakeword/actions/workflows/ci.yml)
[![Version](https://img.shields.io/npm/v/react-native-openwakeword.svg)](https://www.npmjs.com/package/react-native-openwakeword)
[![Platform](https://img.shields.io/badge/platform-ios%20%7C%20android-blue.svg)](https://www.npmjs.com/package/react-native-openwakeword)
[![Downloads](https://img.shields.io/npm/dm/react-native-openwakeword.svg)](https://www.npmjs.com/package/react-native-openwakeword)
[![License](https://img.shields.io/npm/l/react-native-openwakeword.svg)](https://www.npmjs.com/package/react-native-openwakeword)

## Requirements

- React Native **v0.74.0** or higher
- iOS **15.1** or higher
- Android API **24** or higher
- Node **20.0.0** or higher

## Installation

```bash
npm install react-native-openwakeword react-native-nitro-modules
```

## Usage

`react-native-openwakeword` is a wake word inference engine built with Nitro. It handles the machine learning detection. **You must bring your own audio capture library** (such as `react-native-audio-api` or `react-native-live-audio-stream`).

### 1. Preparing the Models
The library uses TensorFlow Lite to run the official [openWakeWord](https://github.com/dscripka/openWakeWord) models. You need three `.tflite` models:
1. `melspectrogram.tflite`
2. `embedding_model.tflite`
3. Your target wake word (e.g., `hey_jarvis.tflite`)

> [!WARNING]  
> **Android Pathing Gotcha**: The C++ inference engine requires **absolute file paths**. You cannot simply `require()` the models because Android compresses them into the APK. You must download the models to the device filesystem at runtime (e.g., using `expo-file-system`) or use an asset manager to copy them to the `DocumentDirectory` so C++ can read them.

```typescript
import { Openwakeword } from 'react-native-openwakeword';
import * as FileSystem from 'expo-file-system';

// Provide absolute paths to the models on the device filesystem
const isLoaded = Openwakeword.loadModels(
  `${FileSystem.documentDirectory}/melspectrogram.tflite`,
  `${FileSystem.documentDirectory}/embedding.tflite`,
  `${FileSystem.documentDirectory}/hey_jarvis.tflite`
);

if (isLoaded) {
    console.log("Models loaded successfully!");
}
```

### 2. Processing Audio Streams
Start your microphone stream and pipe the 16kHz, 16-bit Mono PCM `ArrayBuffer` directly into the engine. The C++ engine handles the complex streaming ring buffers internally.

```typescript
import { AudioApi } from 'react-native-audio-api'; // Example audio library

AudioApi.startRecording(
  { sampleRate: 16000, channels: 1, pcmFormat: 'int16' }, 
  (buffer: ArrayBuffer) => {
    // Zero-copy ingest directly into C++
    const result = Openwakeword.processFrame(buffer);
    
    if (result.isDetected) {
      console.log(`Wake word detected! Probability: ${result.probability}`);
      // Trigger your voice assistant!
    }
  }
);
```

### 3. Resetting the Engine
Because the wake word models are stateful and accumulate context over time, you should clear the internal buffers whenever you manually restart a voice session to prevent "ghost" triggers from old audio.

```typescript
Openwakeword.reset();
```

## Testing and Tuning
The `example/` directory contains a testing environment to:
- **Test**: See real-time feedback for model confidence and room noise.
- **Tune**: Find the right sensitivity threshold for your environment.
- **Benchmark**: Check how your device microphone performs.

## Credits
Bootstrapped with [create-nitro-module](https://github.com/patrickkabwe/create-nitro-module).

## Contributing

Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.
