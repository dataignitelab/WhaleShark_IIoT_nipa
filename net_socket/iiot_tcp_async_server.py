import logging
import sys
import math
import json
import time
from datetime import datetime
import pika
import mongo_manager
from net_socket.signal_killer import GracefulInterruptHandler

logging.basicConfig(format='%(asctime)s %(levelname)-8s %(message)s', stream=sys.stdout, level=logging.DEBUG,
                    datefmt='%Y-%m-%d %H:%M:%S')


def init_facilities_info(redis_con):
    facilities_dict = \
        {
            'TS0001': {
                '0001': 'TS_VOLT1_(RS)',
                '0002': 'TS_VOLT1_(ST)',
                '0003': 'TS_VOLT1_(RT)',
                '0004': 'TS_AMP1_(R)',
                '0005': 'TS_AMP1_(S)',
                '0006': 'TS_AMP1_(T)',
                '0007': 'INNER_PRESS',
                '0008': 'PUMP_PRESS',
                '0009': 'TEMPERATURE1(PV)',
                '0010': 'TEMPERATURE1(SV)',
                '0011': 'OVER_TEMP'
            },
            'TS0002': {
                '0001': 'TS_VOLT1_(RS)',
                '0002': 'TS_VOLT1_(ST)',
                '0003': 'TS_VOLT1_(RT)',
                '0004': 'TS_AMP1_(R)',
                '0005': 'TS_AMP1_(S)',
                '0006': 'TS_AMP1_(T)',
                '0007': 'INNER_PRESS',
                '0008': 'PUMP_PRESS',
                '0009': 'TEMPERATURE1(PV)',
                '0010': 'TEMPERATURE1(SV)',
                '0011': 'OVER_TEMP'
            },
            'TS0003': {
                '0001': 'TS_VOLT1_(RS)',
                '0002': 'TS_VOLT1_(ST)',
                '0003': 'TS_VOLT1_(RT)',
                '0004': 'TS_AMP1_(R)',
                '0005': 'TS_AMP1_(S)',
                '0006': 'TS_AMP1_(T)',
                '0007': 'INNER_PRESS',
                '0008': 'PUMP_PRESS',
                '0009': 'TEMPERATURE1(PV)',
                '0010': 'TEMPERATURE1(SV)',
                '0011': 'OVER_TEMP'
            },
            'TS0008': {
                '0001': 'TS_VOLT1_(RS)',
                '0002': 'TS_VOLT1_(ST)',
                '0003': 'TS_VOLT1_(RT)',
                '0004': 'TS_AMP1_(R)',
                '0005': 'TS_AMP1_(S)',
                '0006': 'TS_AMP1_(T)',
                '0007': 'INNER_PRESS',
                '0008': 'PUMP_PRESS',
                '0009': 'TEMPERATURE1(PV)',
                '0010': 'TEMPERATURE1(SV)',
                '0011': 'OVER_TEMP'
            }
        }
    redis_con.set('facilities_info', json.dumps(facilities_dict))


def get_fac_inf(redis_con):
    fac_daq = {}
    facilities_binary = redis_con.get('facilities_info')
    if facilities_binary is None:
        init_facilities_info(redis_con)
    
    facilities_decoded = facilities_binary.decode()
    facilities_info = json.loads(facilities_decoded)
    equipment_keys = facilities_info.keys()
    for equipment_key in equipment_keys:
        fac_daq[equipment_key] = {}
        for sensor_id in facilities_info[equipment_key].keys():
            sensor_desc = facilities_info[equipment_key][sensor_id]
            if sensor_desc not in fac_daq[equipment_key].keys():
                fac_daq[equipment_key][sensor_desc] = 0.0
    return fac_daq


def config_fac_msg(equipment_id, fac_daq, modbus_udp, redis_fac_info):
    sensor_code = modbus_udp['meta']['sensor_cd']
    sensor_desc = redis_fac_info[equipment_id][sensor_code]
    sensor_value = modbus_udp['meta']['sensor_value']
    decimal_point = modbus_udp['meta']['decimal_point']
    pv = float(sensor_value)  # * math.pow(10, float(decimal_point))
    decimal_point = math.pow(10, float(decimal_point))
    fac_daq[equipment_id]['pub_time'] = modbus_udp['meta']['pub_time']
    fac_daq[equipment_id]['ms_time'] = modbus_udp['meta']['ms_time']
    fac_daq[equipment_id][sensor_desc] = pv / decimal_point
    fac_msg = json.dumps({equipment_id: fac_daq[equipment_id]})
    return fac_msg


class AsyncServer:
    
    def __init__(self, redis_manager):
        self.mongo_mgr = mongo_manager.MongoMgr()
        self.redis_mgr = redis_manager
        
    def convert(self, packet_list):
        return tuple(i for i in packet_list)
    
    def publish_facility_msg(self, mqtt_con, exchange_name, routing_key, json_body):
        try:
            logging.debug('exchange name:' + exchange_name + ' routing key:' + routing_key)
            logging.debug('channel is open:' + str(mqtt_con.is_open))
            if mqtt_con.is_open is False:
                credentials = pika.PlainCredentials('whaleshark', 'whaleshark')
                param = pika.ConnectionParameters('localhost', 5672, '/', credentials)
                connection = pika.BlockingConnection(param)
                mqtt_con = connection.channel()
                mqtt_con.queue_declare(queue=exchange_name)
                mqtt_con.exchange_declare(exchange='facility', exchange_type='fanout')
            
            mqtt_con.basic_publish(exchange=exchange_name, routing_key=routing_key, body=json_body)
            return mqtt_con, json.loads(json_body)
        
        except Exception as e:
            logging.exception(str(e))
            return {'Status': str(e)}
    
    def convert_hex2decimal(self, packet_bytes, host, port, mqtt_valid=True):
        """
        In the packet, the hexadecimal value is converted to a decimal value, structured in json format, and returned.

        packet           TCP Stream packet from IIot Gateway
        readable_sock       client socket object

        packet specification
        stx is the starting code, the hex value matching STX in the ascii code table
        utc time is the time when the sensor value is received from the iiot gate
        equipment id means the id of the equipment and is predefined in the database.
        sensor code is means the sensor's type like as tempeatur, pressure, voltage,...
        decimal_point means the accuracy of sensor value, decimal point.
        The sensor value means the sensor value installed in the facility.
        """
        status = 'ER'
        modbus_dict = {'equipment_id': '', 'meta': {'ip': '',
                                                    'port': '',
                                                    'time': '',
                                                    'sensor_cd': '',
                                                    'fun_cd': '',
                                                    'sensor_value': '',
                                                    'decimal_point': ''
                                                    }}
        try:
            byte_tuple = self.convert(list(packet_bytes))
            logging.debug('byte message\r\n' + str(byte_tuple))
            if byte_tuple[0] == 2 and (byte_tuple[16] == 3 or byte_tuple[18] == 3):
                group = chr(byte_tuple[5]) + chr(byte_tuple[6])
                group_code = int('0x{:02x}'.format(byte_tuple[7]) + '{:02x}'.format(byte_tuple[8]), 16)
                group_code = '{0:04d}'.format(group_code)
                sensor_code = int('0x{:02x}'.format(byte_tuple[9]) + '{:02x}'.format(byte_tuple[10]), 16)
                sensor_code = '{0:04d}'.format(sensor_code)
                fn = chr(byte_tuple[11]) + chr(byte_tuple[12])
                logging.debug('function name:' + fn)
                
                fv = '0x{:02x}'.format(byte_tuple[13]) + '{:02x}'.format(byte_tuple[14]) + '{:02x}'.format(
                    byte_tuple[15]) + '{:02x}'.format(byte_tuple[16])
                decimal_point = int('0x{:02x}'.format(byte_tuple[17]), 16)
                logging.debug('**8Byte pressure:' + str(sensor_code) + ':' + fv)
                fv = int(fv, 16)
                
                ms_time = time.time()
                pub_time = datetime.fromtimestamp(time.time())
                mongo_db_name = 'facility'
                collection = group + group_code
                doc_key = '{:d}-{:02d}-{:02d}'.format(pub_time.year, pub_time.month, pub_time.day)
                pub_time = str(pub_time).replace('.', 'ms')
                if mqtt_valid == True:
                    self.mongo_mgr.document_upsert(mongo_db_name, collection, doc_key, pub_time)
                modbus_dict = {'equipment_id': group + group_code, 'meta': {'ip': host,
                                                                            'port': port,
                                                                            'ms_time': ms_time,
                                                                            'sensor_cd': sensor_code,
                                                                            'fun_cd': fn,
                                                                            'sensor_value': fv,
                                                                            'decimal_point': decimal_point,
                                                                            'pub_time': str(pub_time)
                                                                            }}
                
                status = 'OK'
            else:
                status = 'ER'
        except Exception as e:
            logging.exception(str(e))
        logging.debug(status + str(packet_bytes) + str(modbus_dict))
        return status, str(packet_bytes), modbus_dict
    
    async def get_client(self, event_manger, server_sock, msg_size, rabbit_channel):
        """
        It create client socket with server sockt
        event_manger        It has asyncio event loop
        server_socket       Socket corresponding to the client socket
        msg_size            It means the packet size to be acquired at a time from the client socket.
        msg_queue           It means the queue containing the message transmitted from the gateway.
        """
        with GracefulInterruptHandler() as h:
            client = None
            while True:
                if not h.interrupted:
                    client, _ = await event_manger.sock_accept(server_sock)
                    event_manger.create_task(self.manage_client(event_manger, client, msg_size, rabbit_channel))
                else:
                    client.close()
                    server_sock.close()
                    sys.exit(0)
    
    async def manage_client(self, event_manger, client, msg_size, rabbit_channel):
        """
            It receives modbus data from iiot gateway using client socket.
            event_manger        It has asyncio event loop
            client              It is a client socket that works with multiple iiot gateways.
            msg_size            It means the packet size to be acquired at a time from the client socket.
            msg_queue           It means the queue containing the message transmitted from the gateway.
        """

        fac_daq = get_fac_inf(self.redis_mgr)
        with GracefulInterruptHandler() as h:
            while True:
                if not h.interrupted:
                    try:
                        packet = (await event_manger.sock_recv(client, msg_size))
                    except Exception as e:
                        client.close()
                        logging.debug('Client socket close by exception:' + str(e.args))
                        h.release()
                        break
                    if packet:
                        try:
                            logging.debug('try convert')
                            host, port = client.getpeername()
                            status, packet, modbus_udp = self.convert_hex2decimal(packet, host, port)
                            if status == 'OK':
                                equipment_id = modbus_udp['equipment_id']
                                logging.debug('equipment_id:' + equipment_id)
                                redis_fac_info = json.loads(self.redis_mgr.get('facilities_info'))
                                if equipment_id in redis_fac_info.keys():
                                    logging.debug('config factory message')
                                    fac_msg = config_fac_msg(equipment_id, fac_daq, modbus_udp, redis_fac_info)
                                    
                                    rabbit_channel, rtn_json = self.publish_facility_msg(mqtt_con=rabbit_channel,
                                                                                         exchange_name='facility',
                                                                                         routing_key=equipment_id,
                                                                                         json_body=fac_msg)
                                    if rtn_json == json.loads(fac_msg):
                                        logging.debug(
                                            'mq body:' + str(json.dumps({equipment_id: fac_daq[equipment_id]})))
                                    else:
                                        logging.exception("MQTT Publish Excetion:" + str(rtn_json))
                                        raise NameError('MQTT Publish exception')
                                
                                else:
                                    acq_message = status + packet + 'is not exist equipment_id key\r\n'
                                    logging.debug(acq_message)
                                    client.sendall(acq_message.encode())
                                    continue
                            acq_message = status + packet + '\r\n'
                            logging.debug('rtn:' + acq_message)
                            client.sendall(acq_message.encode())
                        except Exception as e:
                            client.sendall(packet.encode())
                            logging.exception('message error:' + str(e))
                    else:
                        client.close()
                else:
                    client.close()
                    sys.exit(0)
