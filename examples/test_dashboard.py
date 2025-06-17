#!/usr/bin/env python3
import socket
import time
import random
import struct
import threading
import json
import math
from slip_processor import SlipProcessor

class DashboardTester:
    def __init__(self, host='localhost', port=8080):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((host, port))
        self.properties = {}
        self.slip_processor = SlipProcessor()
        self.slip_processor.set_packet_callback(self.process_received_packet)
        self.recv_thread = threading.Thread(target=self.receive_data)
        self.recv_thread.daemon = True
        self.recv_thread.start()
        self.current_values = {
            'temperature': 23.5,
            'pressure': 1013.25,
            'humidity': 45.0,
            'tank_level': 75
        }
        self.oscillation_params = {
            'temperature': {'center': 23.5, 'amplitude': 8.0, 'period': 60.0, 'phase': 0},
            'pressure': {'center': 1013.25, 'amplitude': 25.0, 'period': 45.0, 'phase': 0},
            'humidity': {'center': 45.0, 'amplitude': 20.0, 'period': 90.0, 'phase': 0},
            'tank_level': {'center': 50, 'amplitude': 40, 'period': 120.0, 'phase': 0}
        }
        self.pump_states = {
            'pump1': False,
            'pump2': False
        }
        self.alarm_probability = 0.05
        self.start_time = time.time()
        self.get_property_list()
        time.sleep(1)

    def send_float(self, prop_id, value):
        data = struct.pack('<BBf', 0x02, prop_id, value)
        encoded_data = SlipProcessor.encode_slip(data)
        self.sock.send(encoded_data)

    def send_bool(self, prop_id, value):
        data = struct.pack('<BBB', 0x04, prop_id, 1 if value else 0)
        encoded_data = SlipProcessor.encode_slip(data)
        self.sock.send(encoded_data)

    def send_int(self, prop_id, value):
        data = struct.pack('<BBi', 0x01, prop_id, value)
        encoded_data = SlipProcessor.encode_slip(data)
        self.sock.send(encoded_data)

    def receive_data(self):
        while True:
            try:
                data = self.sock.recv(1024)
                if not data:
                    break
                self.slip_processor.on_data_received(data)
            except Exception as e:
                print(f"Error en receive_data: {e}")
                break

    def process_received_packet(self, packet):
        try:
            if len(packet) >= 2 and packet[0] == 0xFF and packet[1] == 0xFF:
                try:
                    end_idx = packet.index(b'\n', 2)
                    json_data = packet[2:end_idx].decode('utf-8')
                    self.process_property_list(json_data)
                except (ValueError, UnicodeDecodeError) as e:
                    print(f"Error procesando lista de propiedades: {e}")
            else:
                print(f"Paquete recibido: {packet.hex()}")
        except Exception as e:
            print(f"Error procesando paquete: {e}")

    def process_property_list(self, json_data):
        props = json.loads(json_data)
        print("Received properties:")
        for name, info in props.items():
            self.properties[name] = info
            print(f"  {name}: ID={info['id']}, Type={info['type']}")

    def get_property_list(self):
        data = bytes([0x00, 0x00])
        encoded_data = SlipProcessor.encode_slip(data)
        self.sock.send(encoded_data)

    def update_property(self, name, value):
        if name not in self.properties:
            print(f"Unknown property: {name}")
            return
        prop_info = self.properties[name]
        prop_id = prop_info['id']
        prop_type = prop_info['type']
        print(f"Updating property '{name}' (ID: {prop_id}, Type: {prop_type}) with value: {value}")
        if prop_type == 'bool':
            self.send_bool(prop_id, value)
        elif prop_type == 'int':
            self.send_int(prop_id, value)
        elif prop_type in ['double', 'float', 'qreal']:
            self.send_float(prop_id, value)
        else:
            print(f"Unsupported property type: {prop_type}")

    def calculate_smooth_value(self, param_name):
        current_time = time.time()
        elapsed = current_time - self.start_time
        params = self.oscillation_params[param_name]
        main_oscillation = math.sin(2 * math.pi * elapsed / params['period'] + params['phase'])
        secondary_oscillation = 0.3 * math.sin(2 * math.pi * elapsed / (params['period'] / 5))
        noise = random.uniform(-0.1, 0.1)
        combined_oscillation = main_oscillation + secondary_oscillation + noise
        value = params['center'] + params['amplitude'] * combined_oscillation
        if param_name == 'temperature':
            value = max(-10, min(60, value))
        elif param_name == 'pressure':
            value = max(950, min(1080, value))
        elif param_name == 'humidity':
            value = max(0, min(100, value))
        elif param_name == 'tank_level':
            value = max(0, min(100, int(value)))
        return value

    def update_pump_states(self):
        temp = self.current_values['temperature']
        level = self.current_values['tank_level']
        pump1_should_be_on = temp > 30 or level < 30
        pressure = self.current_values['pressure']
        humidity = self.current_values['humidity']
        pump2_should_be_on = pressure < 1000 or humidity > 80
        if pump1_should_be_on and not self.pump_states['pump1']:
            self.pump_states['pump1'] = True
        elif not pump1_should_be_on and self.pump_states['pump1']:
            self.pump_states['pump1'] = random.random() < 0.1
        if pump2_should_be_on and not self.pump_states['pump2']:
            self.pump_states['pump2'] = True
        elif not pump2_should_be_on and self.pump_states['pump2']:
            self.pump_states['pump2'] = random.random() < 0.1

    def should_trigger_alarm(self):
        temp = self.current_values['temperature']
        pressure = self.current_values['pressure']
        humidity = self.current_values['humidity']
        level = self.current_values['tank_level']
        critical_temp = temp > 50 or temp < 0
        critical_pressure = pressure < 980 or pressure > 1050
        critical_humidity = humidity > 90
        critical_level = level < 10
        return critical_temp or critical_pressure or critical_humidity or critical_level or (random.random() < self.alarm_probability)

    def generate_smooth_values(self):
        update_count = 0
        while True:
            if not self.properties:
                print("No properties available. Waiting for property list...")
                time.sleep(1)
                continue
            self.current_values['temperature'] = self.calculate_smooth_value('temperature')
            self.current_values['pressure'] = self.calculate_smooth_value('pressure')
            self.current_values['humidity'] = self.calculate_smooth_value('humidity')
            self.current_values['tank_level'] = self.calculate_smooth_value('tank_level')
            self.update_pump_states()
            alarm_active = self.should_trigger_alarm()
            for name, info in self.properties.items():
                try:
                    if info['type'] in ['double', 'float', 'qreal']:
                        if 'temperature' in name.lower():
                            self.update_property(name, self.current_values['temperature'])
                        elif 'pressure' in name.lower():
                            self.update_property(name, self.current_values['pressure'])
                        elif 'humidity' in name.lower():
                            self.update_property(name, self.current_values['humidity'])
                    elif info['type'] == 'bool':
                        if 'pump1' in name.lower():
                            self.update_property(name, self.pump_states['pump1'])
                        elif 'pump2' in name.lower():
                            self.update_property(name, self.pump_states['pump2'])
                        elif 'alarm' in name.lower():
                            self.update_property(name, alarm_active)
                    elif info['type'] == 'int':
                        if 'level' in name.lower() or 'tank' in name.lower():
                            self.update_property(name, int(self.current_values['tank_level']))
                except Exception as e:
                    print(f"Error updating property {name}: {e}")
            update_count += 1
            if update_count % 10 == 0:
                print(f"\n--- Status Update #{update_count} ---")
                print(f"Temperature: {self.current_values['temperature']:.1f}°C")
                print(f"Pressure: {self.current_values['pressure']:.1f} hPa")
                print(f"Humidity: {self.current_values['humidity']:.1f}%")
                print(f"Tank Level: {int(self.current_values['tank_level'])}%")
                print(f"Pump 1: {'ON' if self.pump_states['pump1'] else 'OFF'}")
                print(f"Pump 2: {'ON' if self.pump_states['pump2'] else 'OFF'}")
                print(f"Alarm: {'ACTIVE' if alarm_active else 'Normal'}")
                print("-" * 30)
            time.sleep(1)

    def run(self):
        print("Starting dashboard test simulation...")
        print("Generating smooth variations for temperature, pressure, humidity, and tank level...")
        try:
            self.generate_smooth_values()
        except KeyboardInterrupt:
            print("\nStopping simulation...")
        except Exception as e:
            print(f"Error: {e}")
        finally:
            self.sock.close()

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description='SCADA Dashboard Tester')
    parser.add_argument('--host', default='localhost', help='Server host')
    parser.add_argument('--port', type=int, default=8080, help='Server port')
    args = parser.parse_args()
    tester = DashboardTester(args.host, args.port)
    tester.run()
