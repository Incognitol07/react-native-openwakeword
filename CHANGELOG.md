## [1.0.1](https://github.com/Incognitol07/react-native-openwakeword/compare/v1.0.0...v1.0.1) (2026-04-29)


### Bug Fixes

* update version to 1.1.0 in package.json ([6de376e](https://github.com/Incognitol07/react-native-openwakeword/commit/6de376e6f5c29764cf768af9320fcb07f51f84a3))

# 1.0.0 (2026-04-29)


### Bug Fixes

* add Node setup step in release workflow ([741355e](https://github.com/Incognitol07/react-native-openwakeword/commit/741355e783f4c4e9eba33f416eb54565f89f970c))
* add NPM_CONFIG_REGISTRY environment variable for semantic-release ([ae70622](https://github.com/Incognitol07/react-native-openwakeword/commit/ae706228eb48dd1b76842316a262928858e0ac85))
* add pod_target_xcconfig for HEADER_SEARCH_PATHS in Openwakeword.podspec and include cstdio in HybridOpenwakeword.cpp ([2f5e1c8](https://github.com/Incognitol07/react-native-openwakeword/commit/2f5e1c8ce76acd03919493182cdd7ac9e12de34f))
* add provenance flag to publish configuration in package.json ([5850605](https://github.com/Incognitol07/react-native-openwakeword/commit/5850605744910d712f1293f633f09c0c62f99cb5))
* add release configuration with branches and plugins in package.json ([5b2da3b](https://github.com/Incognitol07/react-native-openwakeword/commit/5b2da3b13bd36e607451dbd94d631ca46e9d0d7a))
* correct author and repository URLs in package.json ([9ee7a94](https://github.com/Incognitol07/react-native-openwakeword/commit/9ee7a946147133aac3716743c45af169e04289f1))
* correct author and repository URLs in package.json for consistency ([d414697](https://github.com/Incognitol07/react-native-openwakeword/commit/d4146977ad5f138ea551bb5ca8d1dcc061e6633c))
* correct TensorFlowLiteC header path for improved compatibility on Apple platforms ([0e09290](https://github.com/Incognitol07/react-native-openwakeword/commit/0e092907aefa388d64e712ed63eda44e978d80db))
* enhance podspec with C++ interoperability settings and language standard ([bf9af20](https://github.com/Incognitol07/react-native-openwakeword/commit/bf9af2003a8eefda27f9aabe27f29abc14c623a9))
* ensure safe buffer size check in shiftLeft function ([29d69d6](https://github.com/Incognitol07/react-native-openwakeword/commit/29d69d620a92275c72140b1fb9ddbbc8b4b0ad73))
* improve pod_target_xcconfig handling to ensure existing settings are preserved ([6a37e4e](https://github.com/Incognitol07/react-native-openwakeword/commit/6a37e4e272f7d3846b474ca7276168009a269e9b))
* improve README clarity ([2addfef](https://github.com/Incognitol07/react-native-openwakeword/commit/2addfefc77e0ceb00bd67062bb2b36bf77419744))
* move logging macros to the top for better visibility and organization ([8f7f396](https://github.com/Incognitol07/react-native-openwakeword/commit/8f7f3963b706baf1e21ea2f782f5727120e1344e))
* remove redundant 'postcodegen' script from package.json ([f9eeb95](https://github.com/Incognitol07/react-native-openwakeword/commit/f9eeb95cade10c882368b9a161ef8d5a11c75f6a))
* remove unnecessary pipe from build command in iOS workflow and adjust TensorFlow includes for Apple ([336d8ae](https://github.com/Incognitol07/react-native-openwakeword/commit/336d8aee6f8451be0a65b101fcc3992b567c5e24))
* remove unnecessary registry-url from Node setup in CI configuration ([a3dd310](https://github.com/Incognitol07/react-native-openwakeword/commit/a3dd310ca7c5e20df5c8e1e06dd0e1dfb65862a4))
* replace npm with bun for global npm upgrade in CI configuration ([c9b9ca1](https://github.com/Incognitol07/react-native-openwakeword/commit/c9b9ca10598997da9753879006165ae02a83f245))
* streamline release workflow by removing unnecessary permissions and steps ([96884ef](https://github.com/Incognitol07/react-native-openwakeword/commit/96884ef7bcfb3fd3ae233e2e225e13321b18e053))
* update author email in package.json for accuracy ([4581d34](https://github.com/Incognitol07/react-native-openwakeword/commit/4581d342d07f915a544e0c04072ce4a921ec1249))
* update build workflows to use Bun for dependency management and adjust paths for iOS and Android ([0d93d31](https://github.com/Incognitol07/react-native-openwakeword/commit/0d93d3188899d8afa248d78cc4a619293c590fe7))
* update CI configuration for Android build steps and improve dependency setup ([8bdc3fd](https://github.com/Incognitol07/react-native-openwakeword/commit/8bdc3fd4bd55d936387d75cc8ec86acf27b938bb))
* update CI workflow to improve build steps and add caching for CocoaPods ([3e4fba6](https://github.com/Incognitol07/react-native-openwakeword/commit/3e4fba69889528c1b01a4959b7cb499a95ba34b7))
* update iOS build workflow to remove unnecessary pipe from build command and add TAG constant in HybridOpenwakeword class ([8bf3251](https://github.com/Incognitol07/react-native-openwakeword/commit/8bf325142d0540451b7d23b9bc656fa35774e702))
* update iOS simulator destination from iPhone 16 to iPhone 15 in build workflow ([58a43f6](https://github.com/Incognitol07/react-native-openwakeword/commit/58a43f62aa1b9b9cd730786a0b78ad4dfe77c8c0))
* update LogBox warnings and improve StatusOrb and PrecisionClicker components ([7f7a740](https://github.com/Incognitol07/react-native-openwakeword/commit/7f7a7403fdd08e46b67c2efe962c76c449fe914d))
* update package description and keywords for clarity and improved discoverability ([80ca26d](https://github.com/Incognitol07/react-native-openwakeword/commit/80ca26deda5ae93119efc1a3440f257ae633d622))
* update permissions and add registry URL in release workflow ([d738815](https://github.com/Incognitol07/react-native-openwakeword/commit/d738815204d428ca4e2091817c517f75de12644b))
* update pod_target_xcconfig to include TensorFlowLiteC header paths for improved compatibility ([da60129](https://github.com/Incognitol07/react-native-openwakeword/commit/da601293cdfd13d4f304290e018986bde5feb323))
* update pod_target_xcconfig to merge existing settings for improved C++ compatibility ([f5bd8a6](https://github.com/Incognitol07/react-native-openwakeword/commit/f5bd8a6214accd716a6d975f85591806b0801014))
* update podspec to enhance HEADER_SEARCH_PATHS and add C++ flags for improved compatibility ([4c9f03e](https://github.com/Incognitol07/react-native-openwakeword/commit/4c9f03ec2163c5e424fa45fddeca7bd77521292c))
* update README for improved clarity and accuracy in package description and requirements ([c06817f](https://github.com/Incognitol07/react-native-openwakeword/commit/c06817f2d0a01619570e78d8189d72930fe176c3))
* update release process to use npx semantic-release and upgrade npm globally ([728a47e](https://github.com/Incognitol07/react-native-openwakeword/commit/728a47ece32c81058914b21410cb8245d38367f9))
* update version to 1.0.1 and improve repository and bugs field structure in package.json ([b9fd1ad](https://github.com/Incognitol07/react-native-openwakeword/commit/b9fd1ad3c3f3981398431e06c15563266f20c294))
* update working directory for Ruby setup in iOS build workflow ([cc1c5da](https://github.com/Incognitol07/react-native-openwakeword/commit/cc1c5da0fd524218d3e5a0261bf9e94da5da68bd))


### Features

* add model assets ([4acb3f1](https://github.com/Incognitol07/react-native-openwakeword/commit/4acb3f16ad9d10761036c00e41c7d53fd4275ed1))
* add Nitro Codegen step to iOS build workflow and improve logging for Android ([68ccb55](https://github.com/Incognitol07/react-native-openwakeword/commit/68ccb555f7fdf1677afc810527e2203cbf91a4dc))
* enhance App component with new UI elements and audio processing features ([7ffd4fb](https://github.com/Incognitol07/react-native-openwakeword/commit/7ffd4fb8313f0551be7db8ec39a746a12b73d18a))
* implement audio model loading and processing in App component ([2514d17](https://github.com/Incognitol07/react-native-openwakeword/commit/2514d174ca052f1e873ff9f9adc289c638b0b00b))
* update model asset handling and add microphone usage description ([cab1735](https://github.com/Incognitol07/react-native-openwakeword/commit/cab173501e19a90a6e06eb1ff67299f205d1cfd6))
