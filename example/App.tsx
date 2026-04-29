import React, { useEffect, useState, useRef } from 'react';
import {
  Text,
  View,
  StyleSheet,
  PermissionsAndroid,
  Platform,
  Pressable,
  StatusBar,
  Animated,
  Dimensions,
  ScrollView,
  LogBox,
} from 'react-native';
import { SafeAreaProvider, useSafeAreaInsets } from 'react-native-safe-area-context';
import { Openwakeword } from 'react-native-openwakeword';
import LiveAudioStream from 'react-native-live-audio-stream';
import RNFS from 'react-native-fs';
import { Buffer } from 'buffer';

// Suppress legacy library warnings
LogBox.ignoreLogs([
  'new NativeEventEmitter()',
  'Style property \'shadowColor\' is not supported',
]);

// --- Design Tokens (System Pro) ---
const COLORS = {
  background: '#000000', // Pure Black
  surface: '#121212', // Material Surface
  primary: '#007AFF', // System Blue
  success: '#34C759', // System Green
  text: '#FFFFFF',
  textSecondary: '#8E8E93', // System Gray
  border: '#2C2C2E', // System Separator
  error: '#FF3B30', // System Red
};

const SPACING = {
  xs: 4,
  sm: 8,
  md: 16,
  lg: 24,
  xl: 32,
};

const { width: SCREEN_WIDTH } = Dimensions.get('window');

// --- Components ---

const StatusOrb = ({
  isListening,
  detected,
  probability,
}: {
  isListening: boolean;
  detected: boolean;
  probability: number;
}) => {
  const scale = useRef(new Animated.Value(1)).current;
  const opacity = useRef(new Animated.Value(0.4)).current;
  const colorAnim = useRef(new Animated.Value(0)).current; // 0: Ready, 1: Listening, 2: Detected

  // Color & State Transition
  useEffect(() => {
    let target = 0;
    if (detected) target = 2;
    else if (isListening) target = 1;

    Animated.timing(colorAnim, {
      toValue: target,
      duration: 400,
      useNativeDriver: false,
    }).start();
  }, [detected, isListening, colorAnim]);

  // Breathing & Success Effects
  useEffect(() => {
    let animation: Animated.CompositeAnimation | null = null;
    
    if (detected) {
      // Success pop
      Animated.timing(opacity, { toValue: 1, duration: 100, useNativeDriver: true }).start();
      Animated.sequence([
        Animated.spring(scale, { toValue: 1.25, friction: 4, useNativeDriver: true }),
        Animated.spring(scale, { toValue: 1, useNativeDriver: true }),
      ]).start();
    } else if (isListening) {
      // Return to breathing
      Animated.timing(opacity, { toValue: 0.6, duration: 400, useNativeDriver: true }).start(() => {
        animation = Animated.loop(
          Animated.sequence([
            Animated.timing(opacity, { toValue: 0.9, duration: 1200, useNativeDriver: true }),
            Animated.timing(opacity, { toValue: 0.6, duration: 1200, useNativeDriver: true }),
          ])
        );
        animation.start();
      });
    } else {
      // Settle to idle
      Animated.parallel([
        Animated.timing(opacity, { toValue: 0.3, duration: 400, useNativeDriver: true }),
        Animated.spring(scale, { toValue: 1, useNativeDriver: true }),
      ]).start();
    }

    return () => {
      animation?.stop();
    };
  }, [isListening, detected, opacity, scale]);

  const orbColor = colorAnim.interpolate({
    inputRange: [0, 1, 2],
    outputRange: [COLORS.border, COLORS.primary, COLORS.success],
  });

  return (
    <View style={styles.orbContainer}>
      <View style={[styles.ringBase, { borderColor: COLORS.border }]} />
      <Animated.View
        style={[
          styles.ringFill,
          { borderColor: orbColor },
          styles.ringFillMask,
          { transform: [{ rotate: `${probability * 360}deg` }] },
        ]}
      />
      
      <Animated.View
        style={[
          styles.orbWrapper,
          {
            opacity: opacity,
            transform: [{ scale: scale }],
          }
        ]}
      >
        <Animated.View
          style={[
            styles.orb,
            {
              backgroundColor: orbColor,
            },
          ]}
        >
          <Text style={styles.orbStatusText}>
            {detected ? 'Detected!' : isListening ? 'Listening' : 'Ready'}
          </Text>
        </Animated.View>
      </Animated.View>
    </View>
  );
};

const MetricLabel = ({ label, value, color, peak }: { label: string, value: number, color: string, peak?: number }) => (
  <View style={styles.metricItem}>
    <View style={styles.metricHeader}>
      <Text style={styles.metricLabelText}>{label}</Text>
      <Text style={[styles.metricValueText, { color }]}>{(value * 100).toFixed(0)}%</Text>
    </View>
    <View style={styles.metricBarBase}>
      <View style={[styles.metricBarFill, { width: `${value * 100}%`, backgroundColor: color }]} />
      {peak !== undefined && (
        <View style={[styles.peakMarker, { left: `${peak * 100}%`, backgroundColor: color }]} />
      )}
    </View>
  </View>
);

const PrecisionClicker = ({ 
  value, 
  onValueChange 
}: { 
  value: number; 
  onValueChange: (v: number) => void 
}) => {
  const step = 0.05;

  const increment = () => {
    const newValue = Math.min(0.95, value + step);
    onValueChange(newValue);
  };

  const decrement = () => {
    const newValue = Math.max(0.05, value - step);
    onValueChange(newValue);
  };

  return (
    <View style={styles.clickerContainer}>
      <Pressable 
        onPress={decrement}
        style={({ pressed }) => [
          styles.clickerButton,
          pressed && { opacity: 0.7, transform: [{ scale: 0.96 }] }
        ]}
      >
        <Text style={styles.clickerButtonText}>-</Text>
      </Pressable>

      <View style={styles.clickerDisplay}>
        <Text style={styles.clickerValue}>{value.toFixed(2)}</Text>
        <Text style={styles.clickerLabel}>THRESHOLD</Text>
      </View>

      <Pressable 
        onPress={increment}
        style={({ pressed }) => [
          styles.clickerButton,
          pressed && { opacity: 0.7, transform: [{ scale: 0.96 }] }
        ]}
      >
        <Text style={styles.clickerButtonText}>+</Text>
      </Pressable>
    </View>
  );
};

const AppContent = () => {
  const insets = useSafeAreaInsets();
  
  // Engine State
  const [isReady, setIsReady] = useState(false);
  const [isListening, setIsListening] = useState(false);
  const [detected, setDetected] = useState(false);
  
  // Metrics State
  const [probability, setProbability] = useState(0);
  const [peak, setPeak] = useState(0);
  const [noise, setNoise] = useState(0);
  const [sensitivity, setSensitivity] = useState(0.5);

  useEffect(() => {
    async function init() {
      if (Platform.OS === 'android') {
        const granted = await PermissionsAndroid.request(PermissionsAndroid.PERMISSIONS.RECORD_AUDIO);
        if (granted !== PermissionsAndroid.RESULTS.GRANTED) return;
      }

      const copyAsset = async (fileName: string) => {
        const dest = `${RNFS.DocumentDirectoryPath}/${fileName}`;
        if (await RNFS.exists(dest)) await RNFS.unlink(dest);
        await RNFS.copyFileAssets(`models/${fileName}`, dest);
        return dest;
      };

      try {
        const paths = await Promise.all([
          copyAsset('melspectrogram.tflite'),
          copyAsset('embedding_model.tflite'),
          copyAsset('wakeword.tflite'),
        ]);

        if (Openwakeword.loadModels(paths[0], paths[1], paths[2])) {
          setIsReady(true);
        }
      } catch (e) {
        console.error('Initialization failed:', e);
      }
    }
    init();
  }, []);

  const toggleListening = () => {
    if (isListening) {
      LiveAudioStream.stop();
      setIsListening(false);
      setProbability(0);
      setPeak(0);
      setNoise(0);
    } else {
      LiveAudioStream.init({
        sampleRate: 16000,
        channels: 1,
        bitsPerSample: 16,
        audioSource: 1,
        bufferSize: 2560,
        wavFile: '',
      });

      LiveAudioStream.on('data', (data: string) => {
        const chunk = Buffer.from(data, 'base64');
        
        // Noise floor calculation
        let maxSample = 0;
        for (let i = 0; i < chunk.length; i += 2) {
          const sample = Math.abs(chunk.readInt16LE(i));
          if (sample > maxSample) maxSample = sample;
        }
        setNoise(Math.min(maxSample / 16384, 1));

        const result = Openwakeword.processFrame(
          chunk.buffer.slice(chunk.byteOffset, chunk.byteOffset + chunk.byteLength)
        );

        const prob = result.probability;
        setProbability(prob);
        if (prob > peak) setPeak(prob);

        if (prob >= sensitivity && !detected) {
          setDetected(true);
          setTimeout(() => setDetected(false), 2000);
        }
      });

      LiveAudioStream.start();
      setIsListening(true);
    }
  };

  return (
    <View style={styles.container}>
      <StatusBar barStyle="light-content" />
      
      <ScrollView 
        contentContainerStyle={[
          styles.scrollContent, 
          { paddingTop: Math.max(insets.top, SPACING.lg) }
        ]} 
        showsVerticalScrollIndicator={false}
      >
        <View style={styles.instructionCard}>
          <Text style={styles.instructionLabel}>Try saying clearly:</Text>
          <Text style={styles.targetPhrase}>"Alexa"</Text>
          <View style={styles.statusRow}>
            <View style={[styles.statusIndicator, isReady && { backgroundColor: COLORS.success }]} />
            <Text style={styles.statusLabel}>
              {isReady ? 'Ready to use' : 'Loading models...'}
            </Text>
          </View>
        </View>

        <StatusOrb 
          isListening={isListening} 
          detected={detected} 
          probability={probability} 
        />
        
        <View style={styles.metricsContainer}>
          <MetricLabel label="Voice Clarity" value={probability} color={COLORS.primary} peak={peak} />
          <MetricLabel label="Room Noise" value={noise} color={noise > 0.7 ? COLORS.error : COLORS.success} />
        </View>

        <View style={styles.calibrationCard}>
          <PrecisionClicker value={sensitivity} onValueChange={setSensitivity} />
          <Text style={styles.calibrationTip}>
            Lower if it's missing your voice. Raise if it's triggering by mistake.
          </Text>
        </View>
      </ScrollView>

      <View style={[styles.footer, { paddingBottom: Math.max(insets.bottom, SPACING.lg) }]}>
        <Pressable
          onPress={toggleListening}
          disabled={!isReady}
          style={({ pressed }) => [
            styles.primaryButton,
            { backgroundColor: isListening ? COLORS.error : COLORS.primary },
            pressed && { opacity: 0.85, transform: [{ scale: 0.98 }] },
            !isReady && { opacity: 0.3 },
          ]}
        >
          <Text style={styles.buttonText}>
            {isListening ? 'Stop Listening' : 'Start Listening'}
          </Text>
        </Pressable>
        <Text style={styles.versionNote}>react-native-openwakeword</Text>
      </View>
    </View>
  );
};

export default function App() {
  return (
    <SafeAreaProvider>
      <AppContent />
    </SafeAreaProvider>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: COLORS.background,
  },
  scrollContent: {
    paddingHorizontal: SPACING.lg,
    alignItems: 'center',
    paddingBottom: SPACING.xl,
  },
  instructionCard: {
    width: '100%',
    padding: SPACING.lg,
    backgroundColor: COLORS.surface,
    borderRadius: 20,
    borderWidth: 1,
    borderColor: COLORS.border,
    alignItems: 'center',
    marginBottom: SPACING.xl,
  },
  instructionLabel: {
    fontSize: 13,
    color: COLORS.textSecondary,
    fontWeight: '500',
  },
  targetPhrase: {
    fontSize: SCREEN_WIDTH > 400 ? 36 : 32,
    fontWeight: '700',
    color: COLORS.text,
    marginVertical: SPACING.xs,
  },
  statusRow: {
    flexDirection: 'row',
    alignItems: 'center',
    marginTop: SPACING.xs,
    gap: SPACING.sm,
  },
  statusIndicator: {
    width: 8,
    height: 8,
    borderRadius: 4,
    backgroundColor: COLORS.border,
  },
  statusLabel: {
    fontSize: 12,
    color: COLORS.textSecondary,
    fontWeight: '500',
  },
  orbContainer: {
    width: 220,
    height: 220,
    justifyContent: 'center',
    alignItems: 'center',
    marginBottom: SPACING.xl,
  },
  orbWrapper: {
    position: 'absolute',
    justifyContent: 'center',
    alignItems: 'center',
  },
  orb: {
    width: 180,
    height: 180,
    borderRadius: 90,
    justifyContent: 'center',
    alignItems: 'center',
  },
  orbStatusText: {
    fontSize: 17,
    fontWeight: '700',
    color: COLORS.text,
  },
  ringBase: {
    position: 'absolute',
    width: 200,
    height: 200,
    borderRadius: 100,
    borderWidth: 2,
  },
  ringFill: {
    position: 'absolute',
    width: 200,
    height: 200,
    borderRadius: 100,
    borderWidth: 4,
  },
  ringFillMask: {
    borderTopColor: 'transparent',
    borderRightColor: 'transparent',
  },
  metricsContainer: {
    width: '100%',
    backgroundColor: COLORS.surface,
    padding: SPACING.lg,
    borderRadius: 20,
    borderWidth: 1,
    borderColor: COLORS.border,
    marginBottom: SPACING.lg,
    gap: SPACING.lg,
  },
  metricItem: {
    width: '100%',
  },
  metricHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: 8,
  },
  metricLabelText: {
    fontSize: 13,
    color: COLORS.textSecondary,
    fontWeight: '600',
  },
  metricValueText: {
    fontSize: 13,
    fontWeight: '700',
  },
  metricBarBase: {
    height: 8,
    width: '100%',
    backgroundColor: COLORS.background,
    borderRadius: 4,
    overflow: 'hidden',
    position: 'relative',
  },
  metricBarFill: {
    height: '100%',
    borderRadius: 4,
  },
  peakMarker: {
    position: 'absolute',
    width: 2,
    height: '100%',
    opacity: 0.6,
  },
  calibrationCard: {
    width: '100%',
    padding: SPACING.lg,
    backgroundColor: COLORS.surface,
    borderRadius: 20,
    borderWidth: 1,
    borderColor: COLORS.border,
    alignItems: 'center',
  },
  clickerContainer: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    width: '100%',
    marginBottom: SPACING.md,
  },
  clickerButton: {
    width: 64,
    height: 64,
    borderRadius: 32,
    backgroundColor: COLORS.background,
    justifyContent: 'center',
    alignItems: 'center',
    borderWidth: 1,
    borderColor: COLORS.border,
  },
  clickerButtonText: {
    fontSize: 28,
    color: COLORS.primary,
    fontWeight: '600',
  },
  clickerSubLabel: {
    fontSize: 9,
    color: COLORS.textSecondary,
    fontWeight: '600',
    marginTop: -2,
  },
  clickerDisplay: {
    alignItems: 'center',
    flex: 1,
  },
  clickerValue: {
    fontSize: 38,
    fontWeight: '700',
    color: COLORS.text,
  },
  clickerLabel: {
    fontSize: 11,
    color: COLORS.primary,
    letterSpacing: 1,
    fontWeight: '700',
  },
  calibrationTip: {
    fontSize: 12,
    color: COLORS.textSecondary,
    lineHeight: 18,
    textAlign: 'center',
    marginTop: SPACING.sm,
    fontWeight: '500',
  },
  footer: {
    paddingHorizontal: SPACING.lg,
    alignItems: 'center',
    backgroundColor: COLORS.background,
  },
  primaryButton: {
    width: '100%',
    height: 60,
    borderRadius: 16,
    justifyContent: 'center',
    alignItems: 'center',
  },
  buttonText: {
    fontSize: 18,
    fontWeight: '700',
    color: COLORS.text,
  },
  versionNote: {
    fontSize: 13,
    color: COLORS.textSecondary,
    marginTop: SPACING.md,
    fontWeight: '500',
  },
});
