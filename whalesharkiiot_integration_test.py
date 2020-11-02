import time
import unittest
import json
import pika
from iiot_server import TcpServer
from net_socket.iiot_tcp_async_server import AsyncServer, get_fac_inf, config_fac_msg
from iiot_mqtt_agent import Agent
from influxdb import DataFrameClient

class Tcp_server_test(unittest.TestCase):
    
    def system_con(self):
        self.server_ip = 'localhost'
        self.server_port = 1234
        server = TcpServer()
        server.init_config()
        self.mq_channel = server.get_mq_channel()
        self.redis_con = server.get_redis_con()
        self.async_svr = AsyncServer(self.redis_con)
        self.mqtt_agent = Agent()
        self.mqtt_agent.resource_config()
        
    def setUp(self):
        pass
        # rtn=self.redis_con.get('remote_log:iit_server_boot')
        # boot_status=json.loads(rtn)
        # self.server_ip=boot_status['ip']
        # self.server_port=boot_status['port']
        # self.server_stats=boot_status['status']
        # self.assertEqual(self.server_stats,1)
    
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
        _, _, self.modbus_udp = self.async_svr.convert_hex2decimal(packet, self.server_ip, self.server_port)
        del self.modbus_udp['meta']['pub_time']
        del self.modbus_udp['meta']['ms_time']
        
        is_equal = self.modbus_udp == origian_msg
        self.assertEqual(True, is_equal)
    
    def test_02_mqtt_pub_scribe(self):
        self.system_con()
        packet = self.make_packet(facility_id='TS0001', sensor_code='0001', pv=330)
        _, _, modbus_udp = self.async_svr.convert_hex2decimal(packet, self.server_ip, self.server_port)
        fac_daq = get_fac_inf(self.redis_con)
        
        # emit
        credentials = pika.PlainCredentials('whaleshark', 'whaleshark')
        param = pika.ConnectionParameters('localhost', 5672, '/', credentials)
        connection = pika.BlockingConnection(param)
        channel = connection.channel()
        channel.queue_declare(queue='facility')
        channel.exchange_declare(exchange='unittest', exchange_type='fanout')
        
        redis_fac_info = json.loads(self.redis_con.get('facilities_info'))
        equipment_id = modbus_udp['equipment_id']
        if equipment_id in redis_fac_info.keys():
            fac_msg = config_fac_msg(equipment_id, fac_daq, modbus_udp, redis_fac_info)
        channel.basic_publish(exchange='unittest',
                              routing_key='facility',
                              body=fac_msg)
        connection.close()
        
        # recv
        def callback(ch, method, properties, body):
            json_body = json.loads(body)
            json_fac = json.loads(fac_msg)
            json_cmp = json_fac == json_body
            self.assertEqual(json_cmp, True)
            print(json_fac, json_body)
            channel_sub.stop_consuming()
        
        credentials_sub = pika.PlainCredentials('whaleshark', 'whaleshark')
        connection_sub = pika.BlockingConnection(pika.ConnectionParameters('localhost', 5672, '/', credentials_sub))
        channel_sub = connection_sub.channel()
        queue_name = 'facility'  # result.method.queue
        channel_sub.queue_bind(exchange='unittest', queue=queue_name)
        channel_sub.basic_consume(queue='facility', on_message_callback=callback, auto_ack=True)
        channel_sub.start_consuming()
    
    def test_03_influx_wr(self):
        self.system_con()
        influxdb_host = 'localhost'
        influxdb_port = 8086
        influxdb_id = 'krmim'
        influxdb_pwd = 'krmin_2017'
        influxdb_db = 'facility'
        influxdb_client = self.mqtt_agent.get_influxdb_mgr()
        influxdb_Dfclient = DataFrameClient('localhost', 8086, 'krmim', 'krmim_2017', 'facility')
        if influxdb_client == None:
            print('influxdb configuration fail')
        
        packet = self.make_packet(facility_id='TS0001', sensor_code='0001', pv=330)
        _, _, modbus_udp = self.async_svr.convert_hex2decimal(packet, 'localhost', 6379)
        fac_daq = get_fac_inf(self.redis_con)
        redis_fac_info = json.loads(self.redis_con.get('facilities_info'))
        equipment_id = modbus_udp['equipment_id']
        if equipment_id in redis_fac_info.keys():
            fac_msg = config_fac_msg(equipment_id, fac_daq, modbus_udp, redis_fac_info)
        
        fields = {}
        facility_msg_json = json.loads(fac_msg)
        me_timestamp = time.time()
        del facility_msg_json[equipment_id]['ms_time']
        del facility_msg_json[equipment_id]['pub_time']
        for key in facility_msg_json[equipment_id].keys():
            print(key)
            fields[key] = float(facility_msg_json[equipment_id][key])
        
        # fields['me_time'] = me_timestamp
        influx_json = [{
            'measurement': equipment_id,
            'fields': fields
        }]
        try:
            influxdb_Dfclient.query('DROP SERIES FROM TS0001')
            if influxdb_client.write_points(influx_json) is True:
                print('influx write success:' + str(influx_json))
            else:
                print('influx write faile:' + str(influx_json))
        except Exception as e:
            print(str(e.args))
        influxdb_client.close()
        
        ts5_no_coefficient = influxdb_Dfclient.query('SELECT * FROM TS0001 where time >= now() - 15m')
        ts5_no_coefficient = ts5_no_coefficient['TS0001']
        rows_size = ts5_no_coefficient.shape[0]
        row_json = ts5_no_coefficient.to_dict(orient='records')[0]
        cmp = influx_json[0]['fields'] == row_json
        print(cmp)
        print(influx_json[0]['fields'])
        print(row_json)
        self.assertEqual(rows_size, 1)
        influxdb_Dfclient.close()


if __name__ == '__main__':
    unittest.main()
