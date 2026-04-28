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

export interface Openwakeword extends HybridObject<{ ios: 'c++', android: 'c++' }> {
  /**
   * Loads the three required models into the C++ engine memory.
   * @param melspecPath Path to the melspectrogram .tflite/.onnx file
   * @param embeddingPath Path to the embedding .tflite/.onnx file
   * @param wakeWordPath Path to the specific wake word (e.g., 'hey_jarvis.tflite')
   */
  loadModels(
    melspecPath: string, 
    embeddingPath: string, 
    wakeWordPath: string
  ): boolean

  /**
   * Processes a single frame of audio. 
   * Using ArrayBuffer allows zero-copy transfer from the Mic to C++.
   * Expects 16kHz, 16-bit Mono PCM (typically 1280 or 2048 samples per chunk).
   */
  processFrame(buffer: ArrayBuffer): DetectionResult

  /**
   * Sets the sensitivity threshold for detection (default is usually 0.5)
   */
  setThreshold(threshold: number): void

  /**
   * Clears the internal audio buffers/state to start a fresh detection session
   */
  reset(): void
}
