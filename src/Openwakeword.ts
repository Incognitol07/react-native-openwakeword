import { NitroModules } from 'react-native-nitro-modules'
import type { Openwakeword as OpenwakewordSpec, ModelPaths } from './specs/openwakeword.nitro'
import type { WakeWordDetector } from './WakeWordDetector'

const native = NitroModules.createHybridObject<OpenwakewordSpec>('Openwakeword')

/**
 * Entry point for wake word detection.
 */
export const Openwakeword = {
  /**
   * Loads the three models required for wake word detection and resolves
   * with a ready {@linkcode WakeWordDetector}.
   *
   * Runs on a background thread since it involves file I/O and model
   * allocation. Calling this again (e.g. to switch wake words) safely tears
   * down and replaces the previously loaded models.
   * @throws if any model file cannot be read, or fails to parse/allocate.
   */
  async createDetector(paths: ModelPaths): Promise<WakeWordDetector> {
    await native.loadModels(paths)
    return native
  },
}
