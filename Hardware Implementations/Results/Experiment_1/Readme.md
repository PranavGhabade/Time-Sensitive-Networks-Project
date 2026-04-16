# Real-time Traffic Scheduling using IEEE 802.1Qbv (TAPRIO) and 802.1Qav (CBS)

## 1. Objective:
The primary goal of this phase was to implement and evaluate various Time-Sensitive Networking (TSN) scheduling algorithms on Linux hardware to analyse their impact on real-time traffic (Video/Audio) when competing with high-frequency Best-Effort (Text) traffic.

## 2. Experimental Setup:

### · Topology:
Sender (Wired) → Linux Switch (enp1s0) → Access Point (Wired) → Receiver (Wireless).

### · Traffic Generation:
A custom C++ multi-threaded raw socket application was developed to simultaneously inject three traffic classes:

· Video: High Priority (DSCP 46, TC 2) @ 20ms intervals (1400 bytes).  
· Audio: Medium Priority (DSCP 34, TC 1) @ 10ms intervals (1200 bytes).  
· Text: Best Effort (DSCP 10, TC 0) @ 5ms intervals (800 bytes).  

### · Clock Sync:
chronyd was utilised across nodes to approximate time synchronisation.

## 3. Methodology:

· The physical network connections were established strictly according to the defined wired-to-wireless topology.  
· The middle machine (Desktop 2) was configured as a Linux software bridge to act as the central network switch.  
· The QoS scheduling rules were applied on enp1s0, which is the Intel i210 NIC interface connecting the switch to the wireless Access Point.  
· Different TSN algorithms (TAPRIO variations and CBS) were sequentially set up on the switch using tc qdisc commands.  
· The C++ application injected the traffic streams, and the receiver logged the exact arrival times to calculate the inter-arrival gaps.  

## 4. Observation Table: Receiver-Side Performance Metrics

| Scheduling Algorithm | Video Max Delay Spike | Video Jitter (Std Dev) | Text Jitter (Std Dev) | Key Observation |
|----------------------|----------------------|-------------------------|------------------------|----------------|
| Baseline (No QoS) | 53.0 ms | 0.95 ms | 0.67 ms | Severe queue contention; Video frames delayed by Text bursts. |
| Round Robin (1ms equal) | 50.0 ms | 1.36 ms | 1.06 ms | Equal gates punished High Priority traffic; increased jitter. |
| Strict Priority TAPRIO | 41.0 ms | 0.98 ms | 0.92 ms | Best Performance: 2ms gate for Text prevented switch buffer overflow. |
| EDF Approximation | 43.0 ms | 1.15 ms | 0.99 ms | Interleaved gates worked well, but increased hardware context switching. |
| CBS (802.1Qav) | 47.0 ms | 1.30 ms | 0.80 ms | Poor Video shaping due to insufficient locredit configuration for 1400B frames. |

Note: Due to the wireless last-mile connection, absolute End-to-End Latency could not be reliably calculated without hardware PTP timestamping. Therefore, performance was evaluated using Inter-Arrival Time and Jitter.

· Baseline Failure: Fast-arriving text packets filled the switch buffer, causing the highest video delay spike of 53.0 ms.  
· Round Robin Penalty: Giving equal 1ms gates to all queues caused artificial blocking, resulting in the highest video jitter of 1.36 ms.  
· Strict Priority Success: This algorithm gave the best results (41.0 ms delay) because assigning a 2ms gate to the fast Text traffic prevented switch buffer overflow.  
· CBS Limitation: The video stream faced a 47.0 ms delay because the locredit value (-300) was too small for the large 1400-byte video packets.  

---

# TAPRIO CONFIGURATION:

## 1. Round Robin TAPRIO Configuration  
(Equal 1ms gates for all traffic classes)

```bash
sudo tc qdisc replace dev enp1s0 parent root handle 100 taprio \
num_tc 3 \
map 0 1 2 2 2 2 2 2 \
queues 1@0 1@1 1@2 \
base-time 0 \
sched-entry S 0x1 1000000 \
sched-entry S 0x2 1000000 \
sched-entry S 0x4 1000000 \
clockid CLOCK_TAI

## 2. Strict Priority TAPRIO Configuration (Time-Weighted)
(2ms gate for Text [0x1], 1ms for Audio and Video)

```bash
sudo tc qdisc replace dev enp1s0 parent root handle 100 taprio \
num_tc 3 \
map 0 1 2 2 2 2 2 2 \
queues 1@0 1@1 1@2 \
base-time 0 \
sched-entry S 0x1 2000000 \
sched-entry S 0x2 1000000 \
sched-entry S 0x4 1000000 \
clockid CLOCK_TAI

## 3. EDF (Earliest Deadline First) Approximation

```bash
sudo tc qdisc replace dev enp1s0 root handle 100 taprio \
num_tc 3 \
map 0 1 2 2 2 2 2 2 \
queues 1@0 1@1 1@2 \
base-time 0 \
sched-entry S 0x1 1000000 \
sched-entry S 0x2 1000000 \
sched-entry S 0x1 1000000 \
sched-entry S 0x4 1000000 \
clockid CLOCK_TAI

## 4. IEEE 802.1Qav: CBS (Credit-Based Shaper) Configuration

```bash
sudo tc qdisc add dev enp1s0 root handle 100 mqprio \
num_tc 3 \
map 0 1 2 2 2 2 2 2 \
queues 1@0 1@1 1@2 \
hw 0

sudo tc qdisc add dev enp1s0 parent 100:1 cbs \
idleslope 50000 sendslope -50000 hicredit 300 locredit –300

sudo tc qdisc add dev enp1s0 parent 100:2 cbs \
idleslope 20000 sendslope -20000 hicredit 200 locredit -200
