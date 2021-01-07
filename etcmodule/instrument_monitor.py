from __future__ import print_function
import math
import socket
from datetime import datetime
from time import gmtime, strftime
import datetime
import calendar
import time
import minimalmodbus
import yaml
import json
from types import SimpleNamespace as Namespace
from instrument_driver.autonics.instrument_bridge import instruments

client_socket = socket.socket()
host = 'localhost'
port = 1233

class sock_client:

    def __init__(self):
        # self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        # self.client_socket.connect((host, port))
        pass


    def get_timebytearray(self,full_message):
        full_message+=bytearray.fromhex('{:02x}'.format(0))
        full_message+=bytearray.fromhex('{:02x}'.format(0))
        full_message+=bytearray.fromhex('{:02x}'.format(0))
        full_message+=bytearray.fromhex('{:02x}'.format(0))
        return full_message


    def make_packet(self,facility_name,facility_id,fn,fv,decimalpoint):
        hex_stx='{:02x}'.format(2)
        full_message=bytearray.fromhex(hex_stx)
        # timestamp=datetime.datetime.now()
        # unixtime=calendar.timegm(timestamp.utctimetuple())
        # str_hex_unixtime=str(hex(unixtime)).replace('0x','')
        full_message=self.get_timebytearray(full_message)

        facility_name=format(ord(facility_name[0]),"x")+format(ord(facility_name[1]),"x")
        full_message+=bytearray.fromhex(facility_name)

        facility_id='{:02x}'.format(int(facility_id[0]))+'{:02x}'.format(int(facility_id[1]))
        full_message+=bytearray.fromhex(facility_id)

        sensor_code='{:02x}'.format(0)+'{:02x}'.format(9)
        full_message+=bytearray.fromhex(sensor_code)
        fn=(fn[0:1],  fn[1:2])
        function_code=format(ord(fn[0]),"x")+format(ord(fn[1]),"x")
        full_message+=bytearray.fromhex(function_code)

        function_value='{:04x}'.format(fv)
        full_message+=bytearray.fromhex(function_value)

        decimalpoint='{:02x}'.format(decimalpoint)
        full_message+=bytearray.fromhex(decimalpoint)

        hex_etx='{:02x}'.format(3)
        full_message+=bytearray.fromhex(hex_etx)
        print(full_message,len(full_message))
        return full_message


    def send_data(self,facility_name, facility_id, fn, fv, decimalpoint):
        try:
            full_message = self.make_packet(facility_name, facility_id, fn, fv,decimalpoint)
            self.client_socket.send(full_message)
            recv = self.client_socket.recv(1024)
            print("data is ",recv)
            time.sleep(0.5)

        except socket.error as e:
            print(str(e))
            self.client_socket.close()


if __name__ == '__main__':
    slaves_desc={}

    with open('../instrument_driver/config/instrument_desc.json', 'r') as json_file:
        instrument_json=json.load(json_file)

        print(instrument_json)
        for slave in instrument_json['slaves']:
            slaves_desc[slave]=instrument_json[slave]

    slave_list=[]
    for slave_desc in slaves_desc.keys():
        instrument=instruments()
        print(slaves_desc[slave_desc]),
        print(slaves_desc[slave_desc]['vendor'])
        print(slaves_desc[slave_desc]['model_name'])
        print(slaves_desc[slave_desc]['stationid'])
        instrument.connect(slaves_desc[slave_desc])
        slave_list.append(instrument)

    tcp_server=sock_client()
    while True:
        for slave in slave_list:
            if slave.com_type =='serial':
                for fn in slave.function.keys():
                    for sensor_desc in slave.function[fn]:
                        for mem_type in sensor_desc.keys():
                            fv, decimalpoint = 0, 0
                            if len(mem_type) > 0:
                                if mem_type=='decimalpoint':
                                    address=sensor_desc['decimalpoint']
                                    decimalpoint=slave.get_function_value(address)
                                else:
                                    address=sensor_desc[mem_type]
                                    fv=slave.get_function_value(address)
                                    print(fv)
                            # if fv is not None:
                            #     if decimalpoint > 0:
                            #         fv = float(fv) * math.pow(10, decimalpoint)
                            #     tcp_server.send_data(slave.facility_name, slave.facility_id, fn, fv, decimalpoint)
