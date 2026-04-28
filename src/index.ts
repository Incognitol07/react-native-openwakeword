import { NitroModules } from 'react-native-nitro-modules'
import type { Openwakeword as OpenwakewordSpec } from './specs/openwakeword.nitro'

export const Openwakeword =
  NitroModules.createHybridObject<OpenwakewordSpec>('Openwakeword')