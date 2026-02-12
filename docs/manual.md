# X-Phage (.xp0) Language Manual

## Introduction
X-Phage is a lightweight, hardware-optimized programming language designed for high-security environments and next-gen operating systems.

## Project Structure
* **/src**: Contains the compiler source code (`xphage_core.cpp`).
* **/tests**: Storage for all `.xp0` source files.
* **/bin**: Target directory for compiled binaries.
* **/lib**: System libraries and headers.

## Keywords and Syntax
* **pulse core**: Defines the main entry point of the application.
* **shadow**: Declares a secure, encrypted variable.
* **atom**: Declares a standard variable.
* **beam**: Outputs data to the terminal.
* **scan**: Executes a conditional logic block.
* **bypass**: Fallback logic if the scan condition fails.

## Execution Command
To build and run the project, use the provided shell script:
```bash
bash build.sh
