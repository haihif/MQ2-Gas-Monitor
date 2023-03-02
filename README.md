# MQ2-Gas-Monitor

Welcome to the MQ2-Gas-Monitor project! This project is a gas monitoring system that uses the MQ2 sensor to detect and measure various types of gas in the air. The system is designed to provide accurate readings and alert the user when gas levels exceed a certain threshold.

## Getting Started

To get started with the MQ2-Gas-Monitor project, you will need to have the following materials:

MQ2 gas sensor module
ESP32-DEVKIT V1
Breadboard
Jumper wires
LCD display
Buzzer (optional)

## Installation and Setup

Connect the MQ2 gas sensor module to your Arduino board using the breadboard and jumper wires. Refer to the pinout diagram of the MQ2 sensor and your ESP32 board to ensure correct wiring.

Download and install the Arduino IDE on your computer if you haven't already done so. Connect your Arduino board to your computer via USB.

Open the Arduino IDE and create a new sketch. Copy and paste the code from the mq2-gas-monitor.ino file in this repository into the sketch.

Compile the code and upload it to your Arduino board.

Once the code is uploaded, open the Serial Monitor in the Arduino IDE to view the gas readings. The readings will be displayed in parts per million (ppm) for various gases.

Optional: If you want to add visual and auditory alerts when gas levels exceed a certain threshold, you can connect an LED and/or buzzer to your board and modify the code accordingly.

## Usage

The MQ2-Gas-Monitor system is designed to run continuously and provide real-time gas readings. You can adjust the threshold values in the code to trigger alerts when gas levels exceed a certain level. The LED and/or buzzer will turn on to indicate the presence of dangerous gas levels.

## Contributing

If you would like to contribute to the MQ2-Gas-Monitor project, please fork this repository and submit a pull request with your changes. We welcome any contributions that improve the functionality, usability, and reliability of the system.

## License

The MQ2-Gas-Monitor project is licensed under the MIT License. Please refer to the LICENSE file in this repository for more information.
