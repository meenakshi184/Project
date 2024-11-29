#include <iostream>
#include <vector>
#include <random>
#include <iomanip>

// Constants
const double BANDWIDTH = 20e6;  // 20 MHz
const int MODULATION = 8;       // 256-QAM (8 bits per symbol)
const double CODING_RATE = 5.0 / 6.0;
const double DATA_RATE = BANDWIDTH * MODULATION * CODING_RATE;  // bits per second
const int PACKET_SIZE = 8192;   // 1 KB in bits
const double TRANSMISSION_TIME = PACKET_SIZE / DATA_RATE;  // seconds
const double MAX_BACKOFF = 10e-6;  // 10 µs

// Function to simulate the transmission for a given number of users and packets
void simulateWiFi(int users, int packets) {
    std::vector<double> latencies;
    double total_time = 0.0;

    // Random number generator for backoff time
    std::default_random_engine generator;
    std::uniform_real_distribution<double> distribution(0, MAX_BACKOFF);

    for (int i = 0; i < packets; ++i) {
        double latency = 0.0;
        while (true) {
            // Simulate channel checking and backoff
            if ((rand() / (double)RAND_MAX) < (1.0 / users)) {  // Probability the channel is free
                latency += TRANSMISSION_TIME;
                total_time += TRANSMISSION_TIME;
                latencies.push_back(latency);
                break;
            } else {
                double backoff = distribution(generator);
                latency += backoff;
                total_time += backoff;
            }
        }
    }

    // Metrics calculation
    double throughput = (packets * PACKET_SIZE) / total_time;  // bits per second
    double avg_latency = 0.0;
    double max_latency = 0.0;

    for (double l : latencies) {
        avg_latency += l;
        if (l > max_latency) {
            max_latency = l;
        }
    }
    avg_latency /= latencies.size();

    // Print results
    std::cout << "Number of users: " << users << "\n";
    std::cout << "Throughput: " << std::fixed << std::setprecision(2) << (throughput / 1e6) << " Mbps\n";
    std::cout << "Average Latency: " << std::fixed << std::setprecision(6) << (avg_latency * 1e3) << " ms\n";
    std::cout << "Maximum Latency: " << std::fixed << std::setprecision(6) << (max_latency * 1e3) << " ms\n";
}

int main() {
    int packets = 1000;  // Number of packets to simulate

    int user[3] = {1,10,100};

    for(int i = 0 ;i < 3; i++)
    {
        simulateWiFi(user[i], packets);
        std::cout<<std::endl;
    }

    return 0;
}


/*

1. For 1 User and 1 AP

Explanation: With only 1 user, the transmission is straightforward without any
contention, yielding high throughput and minimal latency.

2. For 10 Users and 1 AP

Explanation: With 10 users, there is increased contention leading to some packet 
collisions and backoff periods. This reduces the throughput and increases both average
and maximum latency.

3. For 100 Users and 1 AP

Explanation: When there are 100 users, the channel is heavily congested, leading to 
frequent collisions and long backoff times. This causes a significant decrease in 
throughput and a marked increase in average and maximum latency.


*/
