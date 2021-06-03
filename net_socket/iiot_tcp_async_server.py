import logging
import sys
import math
import json
import time
from datetime import datetime
import pika
from net_socket.signal_killer import GracefulInterruptHandler
import logging
from logging import handlers

logger = logging.getLogger(__name__)
logging.basicConfig(level=logging.DEBUG)
formatter = logging.Formatter('[%(asctime)s-%(name)s-%(levelname)s-%(filename)s:%(lineno)s-%(funcName)10s()]%(message)s')

logHandler = handlers.RotatingFileHandler('log/iiot_tcp_async_debug.log', maxBytes=1, backupCount=0)
logHandler.setLevel(logging.DEBUG)
logHandler.setFormatter(formatter)

errorLogHandler = handlers.RotatingFileHandler('log/iiot_tcp_async_error.log', maxBytes=1, backupCount=0)
errorLogHandler.setLevel(logging.ERROR)
errorLogHandler.setFormatter(formatter)

logger.addHandler(logHandler)
logger.addHandler(errorLogHandler)


def init_facilities_info(redis_con):
    facilities_dict = {
        'TS0001':
            {
                '0001': 'VOLT1_(RS)',
                '0002': 'VOLT1_(ST)',
                '0003': 'VOLT1_(RT)',
                '0004': 'AMP1_(R)',
                '0005': 'AMP1_(S)',
                '0006': 'AMP1_(T)',
                '0007': 'INNER_PRESS',
                '0008': 'PUMP_PRESS',
                '0009': 'TEMPERATURE1(PV)',
                '0010': 'TEMPERATURE1(SV)',
                '0011': 'OVER_TEMP',
                '0012': 'Nitrogen',
                '0013': 'Argon'
            },
        'TS0002':
            {
                '0001': 'VOLT1_(RS)',
                '0002': 'VOLT1_(ST)',
                '0003': 'VOLT1_(RT)',
                '0004': 'AMP1_(R)',
                '0005': 'AMP1_(S)',
                '0006': 'AMP1_(T)',
                '0007': 'INNER_PRESS',
                '0008': 'PUMP_PRESS',
                '0009': 'TEMPERATURE1(PV)',
                '0010': 'TEMPERATURE1(SV)',
                '0011': 'OVER_TEMP',
                '0012': 'Nitrogen',
                '0013': 'Argon'
            },
        'TS0003':
            {
                '0001': 'VOLT1_(RS)',
                '0002': 'VOLT1_(ST)',
                '0003': 'VOLT1_(RT)',
                '0004': 'AMP1_(R)',
                '0005': 'AMP1_(S)',
                '0006': 'AMP1_(T)',
                '0007': 'INNER_PRESS',
                '0008': 'PUMP_PRESS',
                '0009': 'TEMPERATURE1(PV)',
                '0010': 'TEMPERATURE1(SV)',
                '0011': 'OVER_TEMP',
                '0012': 'Nitrogen',
                '0013': 'Argon'
            },
        'TS0004':
            {
                '0001': 'VOLT1_(RS)',
                '0002': 'VOLT1_(ST)',
                '0003': 'VOLT1_(RT)',
                '0004': 'AMP1_(R)',
                '0005': 'AMP1_(S)',
                '0006': 'AMP1_(T)',
                '0007': 'INNER_PRESS',
                '0008': 'PUMP_PRESS',
                '0009': 'TEMPERATURE1(PV)',
                '0010': 'TEMPERATURE1(SV)',
                '0011': 'OVER_TEMP',
                '0012': 'Nitrogen',
                '0013': 'Argon'
            },
        'TS0005':
            {
                '0001': 'VOLT1_(RS)',
                '0002': 'VOLT1_(ST)',
                '0003': 'VOLT1_(RT)',
                '0004': 'AMP1_(R)',
                '0005': 'AMP1_(S)',
                '0006': 'AMP1_(T)',
                '0007': 'INNER_PRESS',
                '0008': 'PUMP_PRESS',
                '0009': 'TEMPERATURE1(PV)',
                '0010': 'TEMPERATURE1(SV)',
                '0011': 'OVER_TEMP',
                '0012': 'Nitrogen',
                '0013': 'Argon'
            },
        'TK0001':
            {
                '0001': 'VOLT1_(RS)',
                '0002': 'VOLT1_(ST)',
                '0003': 'VOLT1_(RT)',
                '0004': 'AMP1_(R)',
                '0005': 'AMP1_(S)',
                '0006': 'AMP1_(T)',
                '0007': 'VOLT2_(RS)',
                '0008': 'VOLT2_(ST)',
                '0009': 'VOLT2_(RT)',
                '0010': 'AMP2_(R)',
                '0011': 'AMP2_(S)',
                '0012': 'AMP2_(T)',
                '0013': 'INNER_PRESS',
                '0014': 'PUMP_PRESS',
                '0015': 'TEMPERATURE1(PV)',
                '0016': 'TEMPERATURE1(SV)',
                '0017': 'TEMPERATURE2(PV)',
                '0018': 'TEMPERATURE2(SV)',
                '0019': 'OVER_TEMP',
                '0020': 'INNER_PRESS2',
                '0021': 'PUMP_PRESS2',
                '0022': 'Nitrogen',
                '0023': 'Argon'
            },
        'TK0002':
            {
                '0001': 'VOLT1_(RS)',
                '0002': 'VOLT1_(ST)',
                '0003': 'VOLT1_(RT)',
                '0004': 'AMP1_(R)',
                '0005': 'AMP1_(S)',
                '0006': 'AMP1_(T)',
                '0007': 'VOLT2_(RS)',
                '0008': 'VOLT2_(ST)',
                '0009': 'VOLT2_(RT)',
                '0010': 'AMP2_(R)',
                '0011': 'AMP2_(S)',
                '0012': 'AMP2_(T)',
                '0013': 'INNER_PRESS',
                '0014': 'PUMP_PRESS',
                '0015': 'TEMPERATURE1(PV)',
                '0016': 'TEMPERATURE1(SV)',
                '0017': 'TEMPERATURE2(PV)',
                '0018': 'TEMPERATURE2(SV)',
                '0019': 'OVER_TEMP',
                '0020': 'INNER_PRESS2',
                '0021': 'PUMP_PRESS2',
                '0022': 'Nitrogen',
                '0023': 'Argon'
            },
        'TK0003':
            {
                '0001': 'VOLT1_(RS)',
                '0002': 'VOLT1_(ST)',
                '0003': 'VOLT1_(RT)',
                '0004': 'AMP1_(R)',
                '0005': 'AMP1_(S)',
                '0006': 'AMP1_(T)',
                '0007': 'VOLT2_(RS)',
                '0008': 'VOLT2_(ST)',
                '0009': 'VOLT2_(RT)',
                '0010': 'AMP2_(R)',
                '0011': 'AMP2_(S)',
                '0012': 'AMP2_(T)',
                '0013': 'INNER_PRESS',
                '0014': 'PUMP_PRESS',
                '0015': 'TEMPERATURE1(PV)',
                '0016': 'TEMPERATURE1(SV)',
                '0017': 'TEMPERATURE2(PV)',
                '0018': 'TEMPERATURE2(SV)',
                '0019': 'OVER_TEMP',
                '0020': 'INNER_PRESS2',
                '0021': 'PUMP_PRESS2',
                '0022': 'Nitrogen',
                '0023': 'Argon'
            },
        'TK0004':
            {
                '0001': 'VOLT1_(RS)',
                '0002': 'VOLT1_(ST)',
                '0003': 'VOLT1_(RT)',
                '0004': 'AMP1_(R)',
                '0005': 'AMP1_(S)',
                '0006': 'AMP1_(T)',
                '0007': 'VOLT2_(RS)',
                '0008': 'VOLT2_(ST)',
                '0009': 'VOLT2_(RT)',
                '0010': 'AMP2_(R)',
                '0011': 'AMP2_(S)',
                '0012': 'AMP2_(T)',
                '0013': 'INNER_PRESS',
                '0014': 'PUMP_PRESS',
                '0015': 'TEMPERATURE1(PV)',
                '0016': 'TEMPERATURE1(SV)',
                '0017': 'TEMPERATURE2(PV)',
                '0018': 'TEMPERATURE2(SV)',
                '0019': 'OVER_TEMP',
                '0020': 'INNER_PRESS2',
                '0021': 'PUMP_PRESS2',
                '0022': 'Nitrogen',
                '0023': 'Argon'
            }
    }
    redis_con.set('facilities_info', json.dumps(facilities_dict))

fac_daq = dict()

def get_fac_inf(redis_con, modbus_udp):
    facilities_binary = redis_con.get('facilities_info')
    if facilities_binary is None:
        logging.debug('redis facility info is None\nreset facility info')
        init_facilities_info(redis_con)
        facilities_binary = redis_con.get('facilities_info')

    logging.debug('redis facility info decoding..')
    facilities_decoded = facilities_binary.decode()
    facilities_info = json.loads(facilities_decoded)
    equipment_keys = facilities_info.keys()

    for equipment_key in equipment_keys:

        if equipment_key not in fac_daq:
            fac_daq[equipment_key] = {}

            for sensor_id in facilities_info[equipment_key].keys():
                sensor_desc = facilities_info[equipment_key][sensor_id]
                if sensor_desc not in fac_daq[equipment_key].keys():
                    fac_daq[equipment_key][sensor_desc] = 0.0
        else:
            if equipment_key == modbus_udp['equipment_id']:
                sensor_id = modbus_udp['meta']['sensor_cd']
                sensor_desc = facilities_info[equipment_key][sensor_id]
                fac_daq[equipment_key][sensor_desc] = modbus_udp['meta']['sensor_value']
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
        # self.mongo_mgr = mongo_manager.MongoMgr()
        self.redis_mgr = redis_manager
        
    def convert(self, packet_list):
        return tuple(i for i in packet_list)
    
    def publish_facility_msg(self, mqtt_con, exchange_name, routing_key, json_body, exchange_type):
        try:
            logging.debug('exchange name:' + exchange_name + ' routing key:' + routing_key)
            logging.debug('channel is open:' + str(mqtt_con.is_open))
            if mqtt_con.is_open is False:
                credentials = pika.PlainCredentials('whaleshark', 'whaleshark')
                param = pika.ConnectionParameters('localhost', 5672, '/', credentials)
                connection = pika.BlockingConnection(param)
                mqtt_con = connection.channel()
                try:
                    mqtt_con.queue_declare(queue=routing_key)
                    mqtt_con.exchange_declare(exchange=exchange_name, exchange_type=exchange_type)
                except Exception as e:
                    logging.exception(str(e))

            mqtt_con.basic_publish(exchange=exchange_name, routing_key=routing_key, body=json_body)
            return mqtt_con, json.loads(json_body)
        
        except Exception as e:
            logger.error(str(e))
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
            logger.debug('byte message')
            logger.debug('1[' + str(packet_bytes)+ ']')
            logger.debug('2['+str(byte_tuple)+']')
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
                logging.debug('**4Byte Function Value:' + str(sensor_code) + ':' + fv)
                fv = int(fv, 16)
                
                ms_time = time.time()
                pub_time = datetime.fromtimestamp(time.time())
                # mongo_db_name = 'facility'
                # collection = group + group_code
                # doc_key = '{:d}-{:02d}-{:02d}'.format(pub_time.year, pub_time.month, pub_time.day)
                pub_time = str(pub_time).replace('.', 'ms')
                # if mqtt_valid == True:
                #     self.mongo_mgr.document_upsert(mongo_db_name, collection, doc_key, pub_time)
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
            logger.error(str(e))
        logger.debug(status + str(packet_bytes) + str(modbus_dict))
        return status, str(packet_bytes), modbus_dict
    
    async def get_client(self, event_manger, server_sock, msg_size, rabbit_channel, exchange_type):
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
                    event_manger.create_task(self.manage_client(event_manger, client, msg_size, rabbit_channel,exchange_type))
                else:
                    client.close()
                    server_sock.close()
                    sys.exit(0)
    
    async def manage_client(self, event_manger, client, msg_size, rabbit_channel, exchange_type):
        """
            It receives modbus data from iiot gateway using client socket.
            event_manger        It has asyncio event loop
            client              It is a client socket that works with multiple iiot gateways.
            msg_size            It means the packet size to be acquired at a time from the client socket.
            msg_queue           It means the queue containing the message transmitted from the gateway.
        """

        with GracefulInterruptHandler() as h:
            while True:
                if not h.interrupted:
                    try:
                        packet = (await event_manger.sock_recv(client, msg_size))
                    except Exception as e:
                        logger.error('Client socket close by exception:' + str(client.getpeername()) + ':' + str(e.args))
                        try:
                            host, port = client.getpeername()
                            logger.error('Info client :'+ host + ' port :' + port)
                        except Exception as e:
                            logger.error('Bug: Could not resolve host, ip info,'+str(e))

                        client.close()
                        h.release()
                        break
                    if packet:
                        try:
                            logger.debug('try convert')
                            host, port = client.getpeername()
                            status, packet, modbus_udp = self.convert_hex2decimal(packet, host, port)
                            fac_daq = get_fac_inf(self.redis_mgr, modbus_udp)
                            if status == 'OK':
                                equipment_id = modbus_udp['equipment_id']
                                logger.debug('equipment_id:' + equipment_id)
                                fi_dict = self.redis_mgr.get('facilities_info')
                                if fi_dict is not None:
                                    redis_fac_info = json.loads(fi_dict)
                                    if equipment_id in redis_fac_info.keys():
                                        logger.debug('config factory message')
                                        fac_msg = config_fac_msg(equipment_id, fac_daq, modbus_udp, redis_fac_info)

                                        rabbit_channel, rtn_json = self.publish_facility_msg(mqtt_con=rabbit_channel,
                                                                                             exchange_name='facility',
                                                                                             routing_key=equipment_id,
                                                                                             json_body=fac_msg,
                                                                                             exchange_type=exchange_type)
                                        if rtn_json == json.loads(fac_msg):
                                            logger.debug(
                                                'mq body:' + str(json.dumps({equipment_id: fac_daq[equipment_id]})))
                                        else:
                                            logger.error("MQTT Publish Excetion:" + str(rtn_json))
                                            raise NameError('MQTT Publish exception')
                                    else:
                                        acq_message = status + packet + 'is not exist equipment_id key\r\n'
                                        logger.debug(acq_message)
                                        client.sendall(acq_message.encode())
                                        continue
                                else:
                                    logger.debug('redis key facilities_info is None, key will be reset')
                                    init_facilities_info(self.redis_mgr)

                            acq_message = status + packet + '\r\n'
                            client.sendall(acq_message.encode())
                        except Exception as e:
                            client.sendall(packet.encode())
                            logger.error('message error:' + str(e))
                    else:
                        client.close()
                else:
                    client.close()
                    sys.exit(0)
