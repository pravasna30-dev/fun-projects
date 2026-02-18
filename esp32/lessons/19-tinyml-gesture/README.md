# Lesson 19: TinyML Gesture Recognition

![Difficulty: Expert](https://img.shields.io/badge/Difficulty-Expert-red)

**Estimated Time: 120-150 minutes**

---

## What You'll Learn

- What TinyML is and why it matters for edge computing
- Reading accelerometer and gyroscope data from the MPU6050 IMU
- Collecting gesture data samples via serial commands
- Building a threshold-based gesture classifier
- The full TinyML pipeline: data collection, training, conversion, deployment
- How TensorFlow Lite Micro runs on microcontrollers
- Where to go next for real neural network deployment

## What You'll Need

| Component | Quantity | Notes |
|-----------|----------|-------|
| XIAO ESP32S3 Sense | 1 | Main board |
| MPU6050 module | 1 | 6-axis accelerometer/gyroscope (I2C) |
| Breadboard | 1 | For connecting the MPU6050 |
| Jumper wires | 4 | Male-to-male or male-to-female |
| USB-C cable | 1 | For programming and serial communication |

## Circuit Diagram

```
                        +-----+
                        | USB |
                   +----+-----+----+
                   |  XIAO ESP32S3 |
                   |    (top view)  |
              D0 --|1            14|-- 5V
              D1 --|2            13|-- GND ---+
              D2 --|3            12|-- 3V3 ---|--+
              D3 --|4            11|-- D10    |  |
   MPU SDA-->D4 --|5            10|-- D9     |  |
   MPU SCL-->D5 --|6             9|-- D8     |  |
              D6 --|7             8|-- D7     |  |
                   +---------------+          |  |
                                              |  |
        MPU6050 Module:                       |  |
        +--------------+                      |  |
        | VCC  --------+----------------------|--+
        | GND  --------+----------------------+
        | SCL  --------+--> D5 (GPIO 7)
        | SDA  --------+--> D4 (GPIO 6)
        | XDA  (NC)    |
        | XCL  (NC)    |
        | AD0  (NC)    |  Leave floating or tie to GND (address 0x68)
        | INT  (NC)    |
        +--------------+

    Wiring Summary:
    MPU6050 VCC -> 3V3
    MPU6050 GND -> GND
    MPU6050 SDA -> D4 (GPIO 6)
    MPU6050 SCL -> D5 (GPIO 7)
```

---

## Step-by-Step Instructions

### Step 1: Understand TinyML

TinyML (Tiny Machine Learning) is the practice of running machine learning models on microcontrollers -- devices with kilobytes of RAM, not gigabytes. This matters because:

- **Privacy** -- Data is processed locally, never leaving the device
- **Latency** -- No network round-trip; decisions happen in milliseconds
- **Power** -- Microcontrollers use milliwatts, not watts
- **Cost** -- A $5 microcontroller replaces a $500 cloud bill
- **Reliability** -- Works without internet connectivity

The ESP32-S3 with 8MB PSRAM is an excellent TinyML platform -- it can run models that would be impossible on simpler microcontrollers.

### Step 2: The Full TinyML Pipeline

Building a real TinyML gesture recognition system involves five stages:

```
+------------------+     +------------------+     +------------------+
| 1. Data          |     | 2. Train Model   |     | 3. Convert to    |
|    Collection    | --> |    (Python/TF)    | --> |    TFLite Micro  |
| (on device)      |     | (on computer)     |     | (quantize)       |
+------------------+     +------------------+     +------------------+
                                                          |
                                                          v
+------------------+     +------------------+
| 5. Run Inference |     | 4. Deploy to     |
|    (real-time)   | <-- |    ESP32         |
+------------------+     +------------------+
```

In this lesson, we will implement stages 1 and 5 on the ESP32. For stage 5, we use a threshold-based classifier as a stand-in for a neural network. The README explains the full pipeline so you understand the complete picture.

### Step 3: Install the MPU6050 Library

Open Arduino IDE and install via **Sketch > Include Library > Manage Libraries**:

1. **Adafruit MPU6050** (also install the prompted dependencies: Adafruit Unified Sensor, Adafruit BusIO)

### Step 4: Wire the MPU6050

Connect the MPU6050 to the XIAO ESP32S3 using the wiring diagram above. The MPU6050 uses I2C, which only requires two data wires (SDA and SCL) plus power.

Double-check:
- VCC goes to **3V3** (not 5V -- while some modules have a regulator, 3V3 is safer)
- SDA and SCL are not swapped

### Step 5: Upload and Test

1. Open `tinyml-gesture.ino`
2. Select board **XIAO_ESP32S3** and port `/dev/cu.usbmodem101`
3. Upload the sketch
4. Open Serial Monitor at 115200 baud
5. You should see accelerometer readings streaming

### Step 6: Collect Gesture Samples

The sketch supports three serial commands for data collection:

| Command | Action |
|---------|--------|
| `p` | Record a "punch" gesture (thrust the sensor forward quickly) |
| `f` | Record a "flex" gesture (tilt the sensor up slowly) |
| `c` | Classify the next gesture using the threshold-based classifier |
| `r` | Print raw accelerometer data continuously |
| `s` | Stop raw data output |

To collect samples:
1. Type `p` and press Enter
2. When you see "Recording in 1 second...", perform a punch gesture
3. Repeat 5-10 times to see the data patterns
4. Do the same with `f` for flex gestures

### Step 7: Test Classification

1. Type `c` and press Enter
2. Perform a gesture
3. The classifier will attempt to identify it as punch, flex, or idle
4. Try different gestures and note the classification results

---

## The Code

### MPU6050 Data Reading

```cpp
sensors_event_t accel, gyro, temp;
mpu.getEvent(&accel, &gyro, &temp);
```

The MPU6050 returns six values: 3-axis acceleration (m/s^2) and 3-axis rotation (rad/s). For gesture recognition, we primarily use acceleration since hand gestures involve distinctive acceleration patterns.

### Gesture Data Collection

The sketch records 50 samples over 1 second (50 Hz sampling rate) when you trigger a recording. Each sample contains 6 values (ax, ay, az, gx, gy, gz), creating a "gesture fingerprint" of 300 data points.

### Threshold-Based Classifier

In a real TinyML project, a neural network would analyze the gesture data. For this lesson, we use a simpler approach:

- **Punch** -- Characterized by a large spike in acceleration along one axis (typically X or Y), exceeding 15 m/s^2
- **Flex** -- Characterized by a slow, steady tilt with moderate acceleration change (5-15 m/s^2) and significant gyroscope rotation
- **Idle** -- All readings stay close to gravity (about 9.8 m/s^2 total) with minimal change

This approach works reasonably well for distinct gestures but would fail for subtle differences -- that is exactly why neural networks are used in production.

### Feature Extraction

Before classification, we extract features from the raw data:
- **Peak acceleration** -- Maximum magnitude across all samples
- **Average acceleration** -- Mean magnitude (helps distinguish fast vs. slow movements)
- **Gyroscope energy** -- Sum of absolute gyroscope values (rotation intensity)

---

## How It Works

### The Complete TinyML Workflow (Beyond This Lesson)

#### Stage 1: Data Collection (This Lesson)
Collect hundreds of gesture samples per class. The more varied the data (different speeds, angles, hand positions), the more robust the model.

#### Stage 2: Training in Python/TensorFlow
```python
# Conceptual example -- not runnable code
import tensorflow as tf

model = tf.keras.Sequential([
    tf.keras.layers.Dense(50, activation='relu', input_shape=(300,)),
    tf.keras.layers.Dense(15, activation='relu'),
    tf.keras.layers.Dense(3, activation='softmax')  # 3 gestures
])
model.compile(optimizer='adam', loss='categorical_crossentropy')
model.fit(training_data, training_labels, epochs=100)
```

A simple dense neural network can classify gestures from the 300-point feature vector. More advanced approaches use CNNs or RNNs.

#### Stage 3: Convert to TFLite Micro
```python
converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]
tflite_model = converter.convert()

# Quantize to int8 for microcontroller efficiency
converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
converter.inference_input_type = tf.int8
converter.inference_output_type = tf.int8
```

Quantization reduces the model from float32 to int8, shrinking it by 4x and speeding up inference significantly on microcontrollers without hardware floating point.

#### Stage 4: Deploy to ESP32
The quantized `.tflite` model is converted to a C array and included in the Arduino sketch. The TensorFlow Lite Micro runtime loads and runs it.

#### Stage 5: Real-Time Inference (This Lesson)
The microcontroller continuously reads the IMU, fills a buffer, and runs the model to classify the gesture -- all in real-time, with no network connection needed.

### Useful Resources

- [TensorFlow Lite for Microcontrollers](https://www.tensorflow.org/lite/microcontrollers) -- Official documentation and tutorials
- [Arduino TensorFlow Lite library](https://github.com/tensorflow/tflite-micro-arduino-examples) -- Ready-to-use Arduino examples
- [TinyML Book by Pete Warden](https://www.oreilly.com/library/view/tinyml/9781492052036/) -- The definitive guide
- [Edge Impulse](https://edgeimpulse.com/) -- Platform that simplifies the entire TinyML pipeline (data collection, training, deployment) with a web UI
- [Google Colab TinyML notebooks](https://colab.research.google.com/) -- Search for "TinyML gesture recognition" for runnable notebooks

---

## Try It Out

1. Upload the sketch and verify MPU6050 readings appear in Serial Monitor
2. Type `r` to see raw data streaming -- tilt and shake the sensor to see values change
3. Type `s` to stop streaming
4. Type `p`, wait for the countdown, then punch forward -- observe the recorded acceleration pattern
5. Type `f`, wait for the countdown, then slowly flex your wrist up -- observe the different pattern
6. Type `c` and perform a punch -- check if it classifies correctly
7. Type `c` and perform a flex -- check classification
8. Type `c` and hold the sensor still -- it should classify as idle

---

## Challenges

1. **Add a new gesture** -- Extend the classifier to recognize a fourth gesture (e.g., "wave" -- moving the sensor side to side). Analyze the data pattern, then add a new threshold rule to the classifier.

2. **Export data for real training** -- Modify the sketch to output gesture samples as CSV over serial. Capture the output in a file, then write a Python script using scikit-learn to train a proper classifier (Random Forest is a good starting point). Compare accuracy with the threshold-based approach.

3. **Deploy a real TFLite model** -- Follow the TensorFlow Lite for Microcontrollers guide to train a gesture model in Python, convert it to a C array, and run it on the ESP32-S3 using the `tflite-micro` library. The ESP32-S3's 8MB PSRAM gives you room for models that would never fit on an Arduino Uno.

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| MPU6050 not found | Check I2C wiring (SDA=D4, SCL=D5). Run an I2C scanner sketch to verify address 0x68. Some modules use 0x69 if AD0 is pulled high. |
| Readings are all zero | The MPU6050 may be in sleep mode. The Adafruit library handles wake-up, but try power-cycling the sensor. |
| Noisy/jumpy readings | Add a 100nF capacitor between VCC and GND on the MPU6050. Keep the sensor away from motors or vibrating surfaces. |
| Classification is inaccurate | The threshold-based classifier is intentionally simple. Make your gestures more distinct (faster punches, slower flexes). Adjust the threshold values in the code. |
| "Punch" and "flex" always confused | Ensure your punch is a sharp, quick motion and your flex is a slow, deliberate tilt. The classifier relies on acceleration magnitude and speed to differentiate them. |
| Serial commands not working | Make sure Serial Monitor is set to "Newline" line ending. Type a single character and press Enter. |

---

## What's Next

In [Lesson 20: Smart Doorbell (Capstone)](../20-smart-doorbell/), we bring everything together in a final graduation project -- a complete smart doorbell system using the camera, microphone, BLE, WiFi, SD card, and web dashboard. It is the culmination of everything you have learned across all 20 lessons.
