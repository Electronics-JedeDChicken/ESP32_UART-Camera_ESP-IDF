# ESP32_ESP-IDF UART Camera

This project utilizes the esp32-camera library by Espressif Systems. Also, although the project uses the ESP-IDF framework is the code was still written in C++. The python receiver utilized the serial library.

This UART Camera allows users to click the button to take a photo then transfer and save it serially to another computer via UART- 

1. The project utilizes a 10kΩ pull-up button w/ debounce and interrupt, and installed ISR (Interrupt Service Routine), instead of usual blocking polling.

2. FreeRTOS task, awaken by ISR, was also used for capturing the photos and sending UART header and image files.

3. A baudrate of 921600 was used for more throughput.

4. The camera sensor was configured to have auto white balance, white balance gain, exposure, and gain.

5. Features reliability by comparing received file sizes (from header) w/ the actual received image bytes, ACKs and NACKs, retransmissions (3 attempts), and timeouts.

Further implementation will use CRC (Cyclic Redundancy Check) and packetization of data to transmit.

*To upload the code to the board-

1. Select the appropriate port, hold the FLASH button and when it starts to write, click the RST button (to reset the board into Firmware Download Mode).

2. When the upload succeeded, clik the RST button again (to reset the board into Execution Mode).

*Then also run the receiver/receiver.py script. The photos will be saved in imgs/.

*References- 

1. ESP-IDF Official Documentation- https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/index.html

2. esp32-camera- https://youtu.be/eot6COwCPF0