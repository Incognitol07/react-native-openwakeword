#!/usr/bin/env node

if (process.env.CI || process.env.npm_config_global) {
  process.exit(0)
}

console.log(
  '😊 Thanks for installing react-native-openwakeword! If this helped you, a star would be truly appreciated: https://github.com/Incognitol07/react-native-openwakeword'
)
