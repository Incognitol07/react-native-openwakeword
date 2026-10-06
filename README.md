# react-native-openwakeword

**Fully offline, on-device wake word detection for React Native.** 

Run **openWakeWord** models directly on **iOS and Android** with TensorFlow Lite, C++, and Nitro JSI. An open-source alternative to cloud wake word APIs and proprietary SDKs like **Picovoice Porcupine** and **DaVoice**.

[![CI](https://github.com/incognitol07/react-native-openwakeword/actions/workflows/ci.yml/badge.svg)](https://github.com/incognitol07/react-native-openwakeword/actions/workflows/ci.yml)
[![Version](https://img.shields.io/npm/v/react-native-openwakeword.svg)](https://www.npmjs.com/package/react-native-openwakeword)
[![Platform](https://img.shields.io/badge/platform-ios%20%7C%20android-blue.svg)](https://www.npmjs.com/package/react-native-openwakeword)
[![Downloads](https://img.shields.io/npm/dm/react-native-openwakeword.svg)](https://www.npmjs.com/package/react-native-openwakeword)
[![License](https://img.shields.io/npm/l/react-native-openwakeword.svg)](https://github.com/Incognitol07/react-native-openwakeword/blob/master/LICENSE)


## The Problem

Building **wake word detection in React Native** often means choosing between cloud services or proprietary SDKs:

- **Picovoice Porcupine:** A commercial, proprietary wake word SDK with its own model and licensing ecosystem.
- **DaVoice:** A third-party React Native wake word solution with its own commercial licensing and implementation.

`react-native-openwakeword` provides an open-source alternative: **run openWakeWord models directly on iOS and Android with TensorFlow Lite, C++, and Nitro JSI.**

This provides full access to the open model ecosystem under a permissive open-source license without third-party commercial blockers.


## Wake Word Detection Comparison

| Requirement | Picovoice Porcupine | DaVoice | **react-native-openwakeword** |
| --- | --- | --- | --- |
| **Offline inference** | ✔️ | Varies | **✔️** |
| **Open source** | ❌ | ❌ | **✔️ Apache-2.0** |
| **Commercial license required** | Yes | Yes | **❌ None** |
| **Bring your own `.tflite` models** | ❌ | Varies | **✔️** |
| **Custom wake word models** | Licensing/model dependent | Varies | **✔️ Compatible `.tflite` models** |
| **Cloud dependency** | Varies | Varies | **❌ None** |
| **API key / account** | Required | Varies | **❌ None** |
| **React Native** | ✔️ | ✔️ | **✔️** |


## Performance

Built with a streaming C++ pipeline and fixed ring buffers, designed to stay safely below real-time audio constraints.

*Tested on a physical budget device (MediaTek MT8781V/CA, Android 16):*

* **Average Frame Inference (`processFrame`):** ~18 ms for an 80 ms audio window *(Real-time budget is ~80 ms)*
* **Steady-State CPU Usage:** ~5% - 7% while listening
* **Memory Footprint:** ~32 MB Native Heap PSS


## Quick Start

### 1. Installation

```sh
npm install react-native-openwakeword react-native-nitro-modules

```

*(Remember to run `cd ios && pod install` for iOS)*

### 2. Usage

Supply your own microphone audio stream (16 kHz, mono, signed 16-bit PCM `ArrayBuffer`) and let the engine handle the rest:

```ts
import { Openwakeword } from 'react-native-openwakeword'

// 1. Initialize detector with absolute paths to your .tflite models
const detector = await Openwakeword.createDetector({
  melspecPath: '/absolute/path/to/melspectrogram.tflite',
  embeddingPath: '/absolute/path/to/embedding_model.tflite',
  wakeWordPath: '/absolute/path/to/hey_jarvis.tflite',
})

detector.setThreshold(0.5)

// 2. Feed raw PCM audio frames from your audio recorder library
function onAudioBuffer(buffer: ArrayBuffer) {
  const result = detector.processFrame(buffer)

  if (result.isDetected) {
    console.log('Wake word detected!', result.probability)
  }
}

```

---

## Requirements & Model Setup

* React Native `0.74.0` or newer
* `react-native-nitro-modules`
* Android API `24`+ | iOS `15.1`+ | Node `20.0.0`+

> **Model flexibility:** Use your own `.tflite` models and control how they are delivered. On Android, copy bundled models or download them into app storage, then pass their absolute paths to the detector.
---

## API Reference

* **`Openwakeword.createDetector(paths: ModelPaths): Promise<WakeWordDetector>`**
Loads the three `.tflite` models on a background thread. Resolves with a ready detector or rejects with a descriptive error if paths are invalid.
* **`WakeWordDetector.processFrame(buffer: ArrayBuffer): DetectionResult`**
Processes incoming PCM audio (expects 1280-sample / 80ms windows at 16kHz) and returns `{ probability: number, isDetected: boolean }`.
* **`WakeWordDetector.setThreshold(threshold: number): void`**
Sets the probability threshold (`0.0` to `1.0`, default `0.5`). Throws if out of bounds.
* **`WakeWordDetector.reset(): void`**
Clears internal streaming audio, mel, and embedding buffers (useful when resetting a voice session).

---

## Profiling & Debugging (Android)

Enable native perf logs via system properties:

```sh
adb shell setprop debug.openwakeword.perf 1
adb logcat -s OpenWakeWord

```

---

## Credits

Built with [Nitro Modules](https://nitro.margelo.com/) and powered by [openWakeWord](https://github.com/dscripka/openWakeWord).

## License

Apache-2.0
