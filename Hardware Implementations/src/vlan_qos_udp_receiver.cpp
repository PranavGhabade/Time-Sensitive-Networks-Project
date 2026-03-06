#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <cstring>
#include <csignal>
#include <ctime>
#include <vector>
#include <chrono>
#include <thread>

#include <pcap.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <net/ethernet.h>
#include <netpacket/packet.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define MAX_PACKET_SIZE 2048

std::ofstream logFile;
pcap_t *pcap_handle = nullptr;
pcap_dumper_t *pcap_dumper = nullptr;
bool running = true;

std::string get_current_timestamp() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    std::time_t t = system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&t), "%F %T") << "." << std::setw(3) << std::setfill('0') << ms.count();
    return oss.str();
}

double parse_sent_timestamp(const std::string& payload) {
    // Example: "[video] #3 @ 2025-07-21 12:34:56.789"
    size_t at_pos = payload.find("@");
    if (at_pos == std::string::npos) return -1;

    std::string ts_str = payload.substr(at_pos + 1);
    std::tm tm{};
    int ms;

    std::istringstream ss(ts_str);
    ss >> std::get_time(&tm, " %Y-%m-%d %H:%M:%S");
    char dot;
    ss >> dot >> ms;

    if (ss.fail()) return -1;

    std::time_t t = std::mktime(&tm);
    return t + (ms / 1000.0);
}

void signal_handler(int) {
    running = false;
}

int main() {
    const char *iface = "wlp0s20f3";
    char errbuf[PCAP_ERRBUF_SIZE];

    // Setup raw socket
    int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    // Bind to interface
    struct ifreq ifr {};
    strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);
    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0) {
        perror("SIOCGIFINDEX");
        return 1;
    }

    sockaddr_ll sll {};
    sll.sll_family = AF_PACKET;
    sll.sll_protocol = htons(ETH_P_ALL);
    sll.sll_ifindex = ifr.ifr_ifindex;

    if (bind(sockfd, (sockaddr *)&sll, sizeof(sll)) < 0) {
        perror("bind");
        return 1;
    }

    // Open PCAP file
    pcap_handle = pcap_open_dead(DLT_EN10MB, MAX_PACKET_SIZE);
    pcap_dumper = pcap_dump_open(pcap_handle, "traffic_capture.pcap");
    if (!pcap_dumper) {
        std::cerr << "Failed to open PCAP file\n";
        return 1;
    }

    // Open CSV log
    logFile.open("traffic_log.csv", std::ios::out);
    logFile << "arrival_time,src_ip,port,payload,latency_ms\n";

    // Setup signal handler
    std::signal(SIGINT, signal_handler);
    std::cout << "Listening on raw socket over " << iface << " (Press Ctrl+C to stop)\n";

    uint8_t buffer[MAX_PACKET_SIZE];

    while (running) {
        ssize_t len = recvfrom(sockfd, buffer, sizeof(buffer), 0, nullptr, nullptr);
        if (len <= 0) continue;
	
        // Dump to PCAP
        struct pcap_pkthdr pkt_hdr{};
        pkt_hdr.len = pkt_hdr.caplen = len;
        gettimeofday(&pkt_hdr.ts, nullptr);
        pcap_dump((u_char *)pcap_dumper, &pkt_hdr, buffer);

        //Changes ABH
        
        if (len < 14) continue; // Ethernet header must exist

	uint16_t ether_type = ntohs(*(uint16_t *)(buffer + 12));
	size_t ip_offset;

	if (ether_type == 0x8100) {
	    if (len < 18) continue; // VLAN tag must exist
	    ether_type = ntohs(*(uint16_t *)(buffer + 16));
	    if (ether_type != 0x0800) continue; // Not IPv4
	    ip_offset = 18;
	} else if (ether_type == 0x0800) {
	    ip_offset = 14;
	} else {
	    continue; // Not IPv4 or VLAN-tagged
	}

	if (len < ip_offset + sizeof(iphdr)) continue;

	iphdr *ip = (iphdr *)(buffer + ip_offset);
	if (ip->protocol != IPPROTO_UDP) continue;

	size_t ip_hdr_len = ip->ihl * 4;
	if (len < ip_offset + ip_hdr_len + sizeof(udphdr)) continue;

	udphdr *udp = (udphdr *)(buffer + ip_offset + ip_hdr_len);
	int udp_len = ntohs(udp->len);
	if (udp_len < sizeof(udphdr) || udp_len > len - ip_offset - ip_hdr_len) continue;

	char *payload = (char *)(buffer + ip_offset + ip_hdr_len + sizeof(udphdr));
	int udp_payload_len = udp_len - sizeof(udphdr);



        	std::string payload_str(payload, udp_payload_len);

        	char src_ip[INET_ADDRSTRLEN];
        	inet_ntop(AF_INET, &ip->saddr, src_ip, sizeof(src_ip));
        	uint16_t dst_port = ntohs(udp->dest);
	std::string arrival_ts = get_current_timestamp();
	std::string data_type="data";
        // Sent timestamp from payload
        double sent_secs = parse_sent_timestamp(payload_str);
        auto now = std::chrono::system_clock::now();
        auto now_secs = std::chrono::duration<double>(now.time_since_epoch()).count();
        double latency_ms = (sent_secs > 0) ? (now_secs - sent_secs) * 1000.0 : -1;

        std::cout << "[" << arrival_ts << "] port " << dst_port << " ← " << src_ip
                  << " | payload: " << payload_str
                  << " | latency: " << std::fixed << std::setprecision(2) << latency_ms << " ms\n";
	if (dst_port==5004 ||dst_port==5006 || dst_port==5008)
	{
		if (dst_port==5004 ) data_type="video";
		if (dst_port==5006 ) data_type="audio";
		if (dst_port==5008 ) data_type="text";
        	logFile << arrival_ts << "," << src_ip << "," << dst_port << ",\""
                << data_type << "\"," << latency_ms << "\n"; 
        }
    
        
        
        
    }

    logFile.close();
    pcap_dump_close(pcap_dumper);
    pcap_close(pcap_handle);
    close(sockfd);
    std::cout << "Receiver stopped.\n";

    return 0;
}
