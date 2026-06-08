"""
Raspberry Pi Actuator Node Script (With CSV Logging)
Function: Receives the command, calculates one-way latency, actuates the servo, 
          and logs the data to a CSV file for Delay and Jitter analysis.
"""
import socket
import time
import csv
import os
import RPi.GPIO as GPIO

# --- Configuration ---
LISTEN_IP = "0.0.0.0"
LISTEN_PORT = 5005
SERVO_PIN = 18
CSV_FILENAME = "pi_latency_log.csv"

# --- GPIO Setup ---
GPIO.setmode(GPIO.BCM)
GPIO.setup(SERVO_PIN, GPIO.OUT)
servo_pwm = GPIO.PWM(SERVO_PIN, 50)
servo_pwm.start(0)

def actuate_servo():
    """Moves the servo motor and resets it."""
    servo_pwm.ChangeDutyCycle(7.5)
    time.sleep(0.5)
    servo_pwm.ChangeDutyCycle(2.5)
    time.sleep(0.5)
    servo_pwm.ChangeDutyCycle(0)

def initialize_csv():
    """Creates the CSV file and writes the header if it doesn't exist."""
    file_exists = os.path.isfile(CSV_FILENAME)
    with open(CSV_FILENAME, mode='a', newline='') as file:
        writer = csv.writer(file)
        if not file_exists:
            # Write the header row
            writer.writerow(["Packet_Count", "Command", "T3_Tx_ns", "T4_Rx_ns", "One_Way_Delay_ms"])
    print(f"[Log] Logging data to {CSV_FILENAME}")

def start_server():
    initialize_csv()
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((LISTEN_IP, LISTEN_PORT))
    
    print(f"Raspberry Pi listening on UDP port {LISTEN_PORT}...")
    
    packet_count = 0
    
    try:
        while True:
            data, addr = sock.recvfrom(1024)
            arrival_time_ns = time.time_ns() # T4
            
            message = data.decode('utf-8')
            
            if "_" in message:
                command, t3_str = message.split("_")
                
                if command == "MOVE":
                    packet_count += 1
                    transmission_time_ns = int(t3_str) # T3
                    
                    # Calculate latency
                    latency_ns = arrival_time_ns - transmission_time_ns
                    latency_ms = latency_ns / 1_000_000.0
                    
                    print(f"[{packet_count}] Delay: {latency_ms:.3f} ms")
                    
                    # Log the exact data to CSV immediately
                    with open(CSV_FILENAME, mode='a', newline='') as file:
                        writer = csv.writer(file)
                        # Rounding to 4 decimal places for precision
                        writer.writerow([packet_count, command, transmission_time_ns, arrival_time_ns, round(latency_ms, 4)])
                    
                    # Actuate motor
                    actuate_servo()
            
    except KeyboardInterrupt:
        print("\nActuator Node stopped.")
    finally:
        servo_pwm.stop()
        GPIO.cleanup()
        sock.close()

if __name__ == "__main__":
    start_server()
