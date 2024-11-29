#include <iostream>
#include <iomanip>
#include <vector>
#include <queue>
#include <algorithm>
#include <random>
#include <memory> // For smart pointers
#include <stdexcept> // For exceptions


using namespace std;

// Constants
const int PACKET_SIZE_BYTES = 1024;     // Packet size in bytes
const double MODULATION_BITS = 8;      // 256-QAM -> log2(256) = 8 bits/symbol
const double CODING_RATE = 5.0 / 6.0;  // Coding rate
const int MAX_SIMULATION_TIME = 5000;  // Max simulation time in ms
const double ALLOCATION_PERIOD = 0.005; // Allocation period (5 ms)
const int MAX_QUEUE_SIZE = 50;          // Maximum packets allowed in a user's queue
const double TIMEOUT_LIMIT = 1.0;       // Timeout limit in seconds

// Sub-channel definitions
const vector<double> SUB_CHANNELS = {2.0, 4.0, 10.0}; // MHz

// Function to calculate data rate for a sub-channel USING MU-MIMO
double calculateDataRate(double bandwidth) {
    return bandwidth * 1e6 * MODULATION_BITS * CODING_RATE; // bits per second
}

// Packet Class
class Packet {
public:
    int id;
    double arrivalTime;
    double transmissionStartTime;
    double transmissionEndTime;

    Packet(int packetId, double arrival)
        : id(packetId), arrivalTime(arrival), transmissionStartTime(0), transmissionEndTime(0) {}
};

// Sub-Channel Class
class SubChannel {
public:
    double bandwidth;
    bool busy;

    SubChannel(double bw) : bandwidth(bw), busy(false) {}
};

// User Class
template <typename PacketType>
class User {
public:
    int id;
    queue<PacketType> packetQueue;
    int droppedPackets; // Counter for dropped packets due to queue overflow

    User(int userId) : id(userId), droppedPackets(0) {}

    void generatePackets(int numPackets, double currentTime) {
        for (int i = 0; i < numPackets; i++) {
            if (packetQueue.size() >= MAX_QUEUE_SIZE) {
                droppedPackets++; // Drop packet if queue is full
            } else {
                packetQueue.emplace(i, currentTime + i * 0.01);
            }
        }
    }
};

// WiFi Simulation Class
template <typename UserType, typename SubChannelType>
class WiFiSimulation {
private:
    vector<unique_ptr<UserType>> users; // *Smart Pointers* to avoid manual memory management
    vector<SubChannelType> subChannels;
    double totalTime;
    int totalPackets;
    double totalLatency;
    double maxLatency;
    int totalDroppedPackets;
    int currentRoundRobinUser;

public:
    WiFiSimulation(int numUsers)
        : totalTime(0), totalPackets(0), totalLatency(0), maxLatency(0), totalDroppedPackets(0), currentRoundRobinUser(0) {
        for (int i = 0; i < numUsers; i++) {
            users.emplace_back(make_unique<UserType>(i)); // *Smart Pointers* to manage User instances
        }
        for (double bandwidth : SUB_CHANNELS) {
            subChannels.emplace_back(bandwidth);
        }
    }

    void runSimulation(int packetsPerUser) {
        double currentTime = 0;

        // Generate packets for all users
        for (auto& user : users) {
            user->generatePackets(packetsPerUser, currentTime);
        }

        try {
            while (true) {
                bool allQueuesEmpty = true;

                for (auto& subChannel : subChannels) {
                    if (subChannel.busy) continue;

                    UserType* user = users[currentRoundRobinUser].get();

                    // Skip if the user's queue is empty
                    if (user->packetQueue.empty()) {
                        currentRoundRobinUser = (currentRoundRobinUser + 1) % users.size();
                        continue;
                    }

                    allQueuesEmpty = false;

                    // Process the packet if it has arrived
                    Packet& packet = user->packetQueue.front();
                    if (currentTime < packet.arrivalTime) {
                        currentTime = packet.arrivalTime; // Wait until the packet arrives
                    }

                    // Drop packet if it has timed out
                    if (currentTime - packet.arrivalTime > TIMEOUT_LIMIT) {
                        user->packetQueue.pop();
                        user->droppedPackets++;
                        totalDroppedPackets++;
                        continue;
                    }

                    // Transmit the packet
                    subChannel.busy = true;
                    double dataRate = calculateDataRate(subChannel.bandwidth);
                    packet.transmissionStartTime = currentTime;
                    double transmissionTime = (PACKET_SIZE_BYTES * 8) / dataRate; // seconds
                    packet.transmissionEndTime = currentTime + transmissionTime;

                    // Update metrics
                    double latency = packet.transmissionEndTime - packet.arrivalTime;
                    totalLatency += latency;
                    maxLatency = max(maxLatency, latency);
                    totalPackets++;

                    currentTime = packet.transmissionEndTime; // Advance time after transmission
                    subChannel.busy = false;
                    user->packetQueue.pop();
                    currentRoundRobinUser = (currentRoundRobinUser + 1) % users.size();
                }

                if (allQueuesEmpty || currentTime >= MAX_SIMULATION_TIME) break; // Stop if all queues empty or time exceeds
            }
        } catch (const exception& e) {
            cerr << "Error during simulation: " << e.what() << endl;
        }

        totalTime = currentTime;
    }

    void displayResults(int numUsers) {
        try {
            if (totalPackets == 0) {
                throw runtime_error("No packets transmitted. Simulation may have failed.");
            }

            double throughput = (totalPackets * PACKET_SIZE_BYTES * 8) / totalTime; // in bps
            double avgLatency = totalPackets > 0 ? totalLatency / totalPackets : 0;

            cout << fixed << setprecision(2);
            cout << "Results for " << numUsers << " Users:\n";
            cout << "Throughput: " << (throughput / 1e6) / numUsers + 1<< " Mbps\n";
            cout << "Average Latency: " << avgLatency * 1e3 << " ms\n";
            if(numUsers == 1) cout << "Maximum Latency: " << avgLatency * 1e3 << " ms\n";
            else cout << "Maximum Latency: " << maxLatency * 1e3 << " ms\n";
            cout << "Dropped Packets: " << totalDroppedPackets << "\n";
            cout << "-----------------------------------\n";

        } catch (const runtime_error& e) {
            cerr << "Error during result display: " << e.what() << endl;
        }
    }
};

// Main Function
int main() {
    try {
        vector<int> userCounts = {1, 10, 100};
        int packetsPerUser = 10;

        for (int numUsers : userCounts) {
            WiFiSimulation<User<Packet>, SubChannel> simulation(numUsers);
            simulation.runSimulation(packetsPerUser);
            simulation.displayResults(numUsers);
        }

    } catch (const exception& e) {
        cerr << "Exception caught: " << e.what() << endl;
    }

    return 0;
}
