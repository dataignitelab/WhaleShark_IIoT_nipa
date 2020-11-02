import unittest

from iiot_mqtt_agent import Agent
from iiot_server import TcpServer
from net_socket.iiot_tcp_async_server import AsyncServer


class whalesharkiiot_unit_test(unittest.TestCase):
    
    def system_con(self):
        self.server_ip = 'localhost'
        self.server_port = 1234
        server = TcpServer()
        server.init_config()
        # self.mq_channel = server.get_mq_channel()
        self.redis_con = server.get_redis_con()
        self.async_svr = AsyncServer(self.redis_con)
        self.mqtt_agent = Agent()
        self.mqtt_agent.resource_config()
    
    def make_packet(self, facility_id, sensor_code, pv):
        hd_fid1 = ord(facility_id[0:1])
        hd_fid2 = ord(facility_id[1:2])
        hd_fid3 = int(facility_id[2:4])
        hd_fid4 = int(facility_id[4:6])
        hd_sid1 = int(sensor_code[0:2])
        hd_sid2 = int(sensor_code[2:4])
        hex_pv = hex(pv)[2:].zfill(8)
        int_pv1 = int(hex_pv[0:2], 16)
        int_pv2 = int(hex_pv[2:4], 16)
        int_pv3 = int(hex_pv[4:6], 16)
        int_pv4 = int(hex_pv[6:8], 16)
        return (
            2, 0, 0, 0, 0, hd_fid1, hd_fid2, hd_fid3, hd_fid4, hd_sid1, hd_sid2, ord('P'), ord('V'), int_pv1, int_pv2,
            int_pv3, int_pv4, 1, 3)
    
    def test_01_hex_conversion(self):
        self.system_con()
        # packet=(2, 0, 0, 0, 0, 84, 83, 0, 1, 0, 9, 80, 86, 0, 0, 1, 74, 1, 3)
        origian_msg = {'equipment_id': 'TS0001',
                       'meta': {'ip': 'localhost', 'port': 1234, 'time': '2020-09-30 13:51:13', 'sensor_cd': '0001',
                                'fun_cd': 'PV', 'sensor_value': 330, 'decimal_point': 1}}
        del origian_msg['meta']['time']
        packet = self.make_packet(facility_id='TS0001', sensor_code='0001', pv=330)
        _, _, self.modbus_udp = self.async_svr.convert_hex2decimal(packet, self.server_ip, self.server_port, mqtt_valid=True)
        del self.modbus_udp['meta']['pub_time']
        del self.modbus_udp['meta']['ms_time']
        
        is_equal = self.modbus_udp == origian_msg
        self.assertEqual(True, is_equal)
   

if __name__ == '__main__':
    unittest.main()
