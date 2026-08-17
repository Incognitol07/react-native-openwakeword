import type { HybridObject } from 'react-native-nitro-modules'

/**
 * Result of a detection attempt.
 */
export interface DetectionResult {
  /** The probability of the wake word (0.0 to 1.0) */
  probability: number
  /** Whether the threshold was officially met */
  isDetected: boolean
}

/**
 * Filesystem paths to the three models required by {@linkcode Openwakeword.loadModels}.
 */
export interface ModelPaths {
  /** Path to the melspectrogram .tflite/.onnx file */
  melspecPath: string
  /** Path to the embedding .tflite/.onnx file */
  embeddingPath: string
  /** Path to the specific wake word model (e.g. 'hey_jarvis.tflite') */
  wakeWordPath: string
}

export interface Openwakeword extends HybridObject<{ ios: 'c++', android: 'c++' }> {
  /**
   * Loads the three required models into the C++ engine memory.
   *
   * Runs on a background thread since it involves file I/O and model
   * allocation; await it before calling {@linkcode Openwakeword.processFrame}.
   * @throws if any model file cannot be read, or fails to parse/allocate.
   */
  loadModels(paths: ModelPaths): Promise<void>

  /**
   * Processes a single frame of audio.
   * Using ArrayBuffer allows zero-copy transfer from the Mic to C++.
   * Expects 16kHz, 16-bit Mono PCM (typically 1280 or 2048 samples per chunk).
   */
  processFrame(buffer: ArrayBuffer): DetectionResult

  /**
   * Sets the sensitivity threshold for detection.
   * @default 0.5
   * @throws if `threshold` is outside the 0.0–1.0 range.
   */
  setThreshold(threshold: number): void

  /**
   * Clears the internal audio buffers/state to start a fresh detection session
   */
  reset(): void
}
