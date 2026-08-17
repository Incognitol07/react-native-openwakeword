import type { DetectionResult } from './specs/openwakeword.nitro'

/**
 * A ready-to-use wake word detector returned by {@linkcode Openwakeword.createDetector}.
 *
 * Only exposes the operations that are valid once models are loaded — there
 * is no way to obtain a `WakeWordDetector` whose models are not yet ready.
 */
export interface WakeWordDetector {
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
