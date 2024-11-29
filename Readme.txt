# References used are:
1. Chat-gpt
2. Gemini
3. Youtube
# WiFi Simulator Project

This project is a C++ command-line simulator designed to model and evaluate the behavior of WiFi communication under various standards, including WiFi 4, WiFi 5, and WiFi 6. The simulator employs object-oriented programming concepts such as inheritance, data abstraction, data hiding, and polymorphism to ensure modularity and scalability.

## Project Goals

1. *WiFi 4 Simulation:*  
   Simulate communication via a single access point (AP) and users. Analyze throughput, average latency, and maximum latency under these conditions:
   - 1 user and 1 AP.
   - 10 users and 1 AP.
   - 100 users and 1 AP.  
   Assumptions:
   - 20 MHz bandwidth.
   - 256-QAM modulation.
   - Coding rate: 5/6.
   - Packet size: 1 KB.

2. *WiFi 5 Simulation:*  
   Simulate multi-user MIMO communication with parallel transmission after initial setup. Analyze throughput, average latency, and maximum latency under the following conditions:
   - Broadcast packet transmission.
   - Sequential channel state information (200 bytes per user).  
   - Parallel communication for 15 ms.  
   Use round-robin scheduling to manage users.

3. *WiFi 6 Simulation:*  
   Simulate OFDMA communication with subchannel allocation. Analyze throughput, average latency, and maximum latency under these conditions:
   - Subchannel widths: 2 MHz, 4 MHz, and 10 MHz.
   - Parallel transmission for 5 ms.  
   Use round-robin scheduling to allocate channels.

---

## Project Structure

- *Source Files:* Contains the main C++ files implementing the simulator logic.
- *Makefile:* Automates the compilation process for both debug and release builds.

---

## Prerequisites

- *C++ Compiler:* Ensure g++ or an equivalent compiler is installed.
- *Make:* Install make for managing build processes.

---

## Compilation

The project supports this build configuration:  
- *Debug Build:* Includes debugging symbols for development and testing.  
 

### Steps to Compile:

1. Open a terminal in the project directory.  
2. Run the following command to compile both builds:

   ```bash
   make
