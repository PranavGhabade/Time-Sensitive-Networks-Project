# This folder contains the source code for the UDP Send and Receive Applications  

## Deploying the Source  
1. Quick Sender working without data files : vlan_qos_udp_sender.cpp  (Dependencies : json , pthread)  
2. Receiver working without any config files (Note: the Wireless interface name needs to be corrected before build) : vlan_qos_udp_receiver.cpp (Dependencies : pcap)  
3. Sender working with data from files : modified_vlan_qos_sender.cpp (Dependencies : json, pthread)  
4. Configuration file containing IP Address, MAC addresses and such interface details: qos_udp_config_updated.json

### Deploy Sender (In Wired Host: Sender PC)    
1. Create a Sender folder at HOME and copy the vlan_qos_udp_sender.cpp and modified_vlan_qos_sender.cpp
2. Copy the configuration file : qos_udp_config_updated.json to the same folder
3. Copy all files video.dat, audio.dat, text.dat found in data to the same Sender folder. If in case these files are huge, we can
   also create them using dd command in linux.  (Size 1400, count 5000)  
   dd if=/dev/urandom of=video.dat bs=1400 count=5000  
   dd if=/dev/urandom of=audio.dat bs=1200 count=5000  
   dd if=/dev/urandom of=text.dat bs=800 count=5000  
4. Update the IP Address, MAC Addresss, Interface Details and the timing details in the configuration file.  
5. Pre-requisites : (json library, pthread library )   
   sudo apt-get install libpthread-stubs0-dev   
   sudo apt-get install nlohmann-json3-dev  
6. Build quick sender : (You may skip this if you are using main sender)   
   g++ vlan_qos_udp_sender.cpp -o vlan_qos_udp_sender -lpthread  
   If build is successful you will find a  vlan_qos_udp_sender executable. Execute it by running:  
   sudo ./vlan_qos_udp_sender  
7. Build Sender :  
   g++ modified_vlan_qos_sender.cpp -o modified_vlan_qos_sender -lpthread 
   If build is successful you will find a  vlan_qos_udp_sender executable. Execute it by running:    
   sudo ./modified_vlan_qos_sender  


### Deploy Receiver (In Wireless Host : Receiver PC)    
1. Create a Receiver folder at HOME and copy the vlan_qos_udp_receiver.cpp
2. Make sure the line 66 is pointing to your WiFi interface ( const char *iface = "<wlp_yourinterface>";)    
3. Build Receiver :  
   g++ vlan_qos_udp_receiver.cpp -o vlan_qos_udp_receiver  
   If build is successful you will find a  vlan_qos_udp_receiver executable. Execute it by running:    
   sudo ./vlan_qos_udp_receiver

## Note: Recheck the IP Addressess, MAC Addressess and Interface details if its not reaching.  Or ping from sender to receiver to check if the network is setup correctly.  


