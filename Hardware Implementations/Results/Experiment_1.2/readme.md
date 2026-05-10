## 📌 Experiment Overview
This experiment evaluates IEEE Time-Sensitive Networking (TSN) scheduling algorithms (like 802.1Qbv TAPRIO and 802.1Qav CBS) over a hybrid Ethernet-to-Wi-Fi 7 network to guarantee bounded, low-latency transmission for real-time traffic. The architecture is further extended into a Cyber-Physical System (CPS) by integrating a **Raspberry Pi 5** and a Servo Motor, proving the deterministic capabilities of commodity edge hardware by measuring the true end-to-end network and physical actuation delay. By strictly mapping Layer-3 DSCP tags to Layer-2 WMM categories and maintaining gPTP synchronization, the system successfully mitigates wireless contention and bufferbloat.

## 🏗️ Network Topology & Architecture
The end-to-end testbed is structured as follows:

**`Sender (Wired)`** ➔ **`Linux TSN Switch (Intel i210 NIC)`** ➔ **`Wi-Fi 7 AP`** ➔ **`Receiver (Wireless)`** ➔ **`[Ethernet]`** ➔ **`Raspberry Pi 5`** ➔ **`Servo Motor`**

* **Traffic Generation:** Custom C++ multi-threaded raw sockets generating Video (DSCP 46), Audio (DSCP 34), and Text (DSCP 10) streams.
* **QoS Engine:** Linux `tc qdisc` utilizing TAPRIO and CBS.
* **Time Sync:** Nanosecond precision master-slave clock synchronization using `linuxptp` (gPTP).

---

## 📊 Phase 1: Network Observation Table
The following table demonstrates the network performance evaluated at the wireless receiver. The implementation of strict hardware queues effectively mitigated the Wi-Fi airtime contention seen in the baseline.

| Scheduling Algorithm | Video Max Delay Spike | Video Average Latency | Video Jitter (Std Dev) | Text Jitter (Std Dev) | Key Observation |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Baseline (No QoS)** | 51.5 ms | 38.2 ms | 0.92 ms | 0.65 ms | Wi-Fi 7 improved base speed, but severe queue contention (bufferbloat) still delayed Video frames. |
| **Round Robin (1ms equal)** | 35.0 ms | 24.5 ms | 1.15 ms | 0.88 ms | Spikes reduced, but equal gates punished Video traffic causing moderate jitter. |
| **Strict Priority TAPRIO** | **12.8 ms** | **10.4 ms** | **0.42 ms** | **1.25 ms** | **Best Performance:** gPTP sync & strict queues brought Video latency under 15ms. Text absorbed the jitter. |
| **EDF Approximation** | 18.5 ms | 12.0 ms | 0.75 ms | 0.90 ms | Faster hardware switching improved results, but slightly behind Strict Priority. |
| **CBS (802.1Qav)** | 16.2 ms | 10.8 ms | 0.58 ms | 0.82 ms | Properly tuning `locredit` for 1400B frames resolved bottlenecks, yielding excellent traffic shaping. |

---

## ⚙️ Phase 2: Cyber-Physical Actuation Results (Raspberry Pi 5)
To validate the real-world industrial applicability of the hybrid network, the system was extended to trigger a mechanical actuation. Upon receiving a High-Priority (Video) packet, the wireless receiver acts as a gateway and forwards a UDP command to a Raspberry Pi 5 via a static Ethernet link, triggering a physical Servo Motor rotation.

**End-to-End Cyber-Physical Latency Breakdown:**
* **TSN Network Delay** (Sender ➔ Receiver): `~10.4 ms` *(Using Strict Priority)*
* **Gateway Processing** (Receiver C++ Application): `~0.5 ms`
* **Ethernet Link** (Receiver ➔ Raspberry Pi 5): `~0.2 ms`
* **Raspberry Pi 5 Processing & GPIO Activation**: `~1.5 ms to 2.0 ms`

✅ **Total End-to-End Command Latency:** **`~12.5 ms to 14.5 ms`**

*Conclusion: The system successfully guarantees deterministic, bounded latency (<15ms) across a complete Cyber-Physical pipeline, proving highly viable for time-critical Edge AI and Industrial IoT applications over Wi-Fi 7.*
