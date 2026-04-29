import React, { useEffect, useState } from 'react';
import { Text, View, StyleSheet, PermissionsAndroid, Platform, Button } from 'react-native';
import { Openwakeword } from 'react-native-openwakeword';
import LiveAudioStream from 'react-native-live-audio-stream';
import RNFS from 'react-native-fs';
import { Buffer } from 'buffer';

export default function App(): React.JSX.Element {
  const [isReady, setIsReady] = useState(false);
  const [isListening, setIsListening] = useState(false);
  const [detected, setDetected] = useState(false);
  const [probability, setProbability] = useState(0);

  useEffect(() => {
    async function init() {
      // 1. Request Mic Permission
      if (Platform.OS === 'android') {
        const granted = await PermissionsAndroid.request(
          PermissionsAndroid.PERMISSIONS.RECORD_AUDIO
        );
        if (granted !== PermissionsAndroid.RESULTS.GRANTED) {
          console.warn('Mic permission denied');
          return;
        }
      }

      // 2. Copy models from assets to file system
      const copyAsset = async (fileName: string) => {
        const dest = `${RNFS.DocumentDirectoryPath}/${fileName}`;
        const exists = await RNFS.exists(dest);
        if (exists) {
          await RNFS.unlink(dest); // Force fresh copy for development to avoid cached bad files
        }
        await RNFS.copyFileAssets(`models/${fileName}`, dest);
        const stat = await RNFS.stat(dest);
        console.log(`[DEBUG] Copied ${fileName}. File exists: true, Size: ${stat.size} bytes. Path: ${dest}`);
        return dest;
      };

      try {
        const melspecPath = await copyAsset('melspectrogram.tflite');
        const embeddingPath = await copyAsset('embedding_model.tflite');
        const wakewordPath = await copyAsset('wakeword.tflite');

        // 3. Load models into C++
        const success = Openwakeword.loadModels(melspecPath, embeddingPath, wakewordPath);
        if (success) {
          setIsReady(true);
        } else {
          console.error('Failed to load OpenWakeWord models');
        }
      } catch (e) {
        console.error('Error copying models:', e);
      }
    }

    init();
  }, []);

  const toggleListening = () => {
    if (isListening) {
      LiveAudioStream.stop();
      setIsListening(false);
    } else {
      // Configure audio stream
      LiveAudioStream.init({
        sampleRate: 16000,
        channels: 1,
        bitsPerSample: 16,
        audioSource: 1, // MIC (Rawer signal, usually higher gain)
        bufferSize: 2560, // 1280 samples * 2 bytes/sample
        wavFile: '', // Required by TS definitions
      });

      LiveAudioStream.on('data', (data: string) => {
        // Convert base64 to ArrayBuffer
        const chunk = Buffer.from(data, 'base64');
        const arrayBuffer = chunk.buffer.slice(chunk.byteOffset, chunk.byteOffset + chunk.byteLength);
        
        // Pass to C++ for detection
        const result = Openwakeword.processFrame(arrayBuffer);
        
        setProbability(result.probability);
        if (result.isDetected) {
          setDetected(true);
          setTimeout(() => setDetected(false), 2000); // Reset UI after 2s
        }
      });

      LiveAudioStream.start();
      setIsListening(true);
    }
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>OpenWakeWord Demo</Text>
      <Text style={styles.status}>
        Models Loaded: {isReady ? '✅' : '❌'}
      </Text>
      
      {isReady && (
        <View style={styles.controls}>
          <Button 
            title={isListening ? "Stop Listening" : "Start Listening"} 
            onPress={toggleListening} 
          />
          
          <View style={[styles.indicator, detected ? styles.detected : styles.idle]}>
            <Text style={styles.indicatorText}>
              {detected ? 'WAKE WORD DETECTED!' : 'Listening...'}
            </Text>
          </View>
          
          <Text style={styles.probability}>
            Probability: {(probability * 100).toFixed(2)}%
          </Text>
        </View>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
    padding: 20,
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    marginBottom: 20,
  },
  status: {
    fontSize: 16,
    marginBottom: 40,
  },
  controls: {
    alignItems: 'center',
    width: '100%',
  },
  indicator: {
    marginTop: 40,
    width: '100%',
    padding: 30,
    borderRadius: 10,
    alignItems: 'center',
  },
  idle: {
    backgroundColor: '#e0e0e0',
  },
  detected: {
    backgroundColor: '#4caf50',
  },
  indicatorText: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#333',
  },
  probability: {
    marginTop: 20,
    fontSize: 16,
    color: '#666',
  }
});