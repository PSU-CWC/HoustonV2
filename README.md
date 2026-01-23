# Houston
Houston is a C++ based dashboard for monitoring microcontrollers during operation. I built it to monitor my university's
wind turbine, but it can be used for any board that supports serial data.

![img.png](src/resource/img.png)

### Building the Code
You can compile this project on Windows, macOS, or Linux using the terminal. Ensure you have CMake and a C++ compiler (like GCC, Clang, or MSVC) installed.

Run the following command to generate the build files. This creates a build/ directory and checks your system for the necessary graphics and communication libraries.  
```cmake -B build -S .```.  

Once the configuration is complete, run the build command. This transforms the C++ source code into a runnable executable.  
```cmake --build build -j 4```.  

### Using the Code
#### Find the ESP32 Port
Windows
* Connect the ESP32 to your computer via USB.
* Right-click the Start button and select Device Manager.
* Expand the Ports (COM & LPT) section.
* Look for "Silicon Labs CP210x" or "USB-SERIAL CH340".
* Your port will look like: COM3.

macOS
* Open the Terminal.
* Run the command: ls /dev/cu.*
* Look for a device containing usbserial or slab.
* Your port will look like: /dev/cu.usbserial-1410.

Linux
* Open the Terminal.
* Run the command: dmesg | grep tty
* Look for "attached to ttyUSB" at the bottom of the output.
* Your port will look like: /dev/ttyUSB0 or /dev/ttyUSB1.

#### Run the code
Run the executable file that you made.  
In the Control Panel section type in the port and click attach.

## ToDo
* Live Graphs
* Better Multithreading
* ~~Save Data to File~~
* ~~Live Variables~~

## Work in progress Documentation
### Protocol Specifications

### Modify Packet
This is used to modify a value within the system

* [Format](https://docs.python.org/3/library/struct.html): <BH3s[f/I]I
    * B: packet id, used to specify type of action
    * H: Number of bytes contained in string data. For modify packet:3
    * [#]s: First byte will specify numeric data's type. F for float, I for integer,B for boolean. The next 2 byte will
      be the variable name to change
    * [f/I]: The actual numeric data specified by previous, boolean still uses I
    * I: checksum of 3s[f/I] data

