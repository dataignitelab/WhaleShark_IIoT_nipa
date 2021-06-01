import asyncio
import socket
import logging
import redis
import yaml
import sys
import json
import pika
from net_socket.iiot_tcp_async_server import AsyncServer

import logging
import logging.handlers as handlers

logger = logging.getLogger('iiot_server')
FORMAT = "[%(filename)s:%(lineno)s - %(funcName)20s() ] %(message)s"
logging.basicConfig(format=FORMAT)
logger.setLevel(logging.DEBUG)

## Here we define our formatter
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')

logHandler = handlers.TimedRotatingFileHandler('log/iiot_server_debug.log', when='M', interval=1, backupCount=0)
logHandler.setLevel(logging.DEBUG)
logHandler.setFormatter(formatter)

errorLogHandler = handlers.RotatingFileHandler('log/iiot_server_error.log', maxBytes=5000, backupCount=0)
errorLogHandler.setLevel(logging.ERROR)
errorLogHandler.setFormatter(formatter)

logger.addHandler(logHandler)
logger.addHandler(errorLogHandler)
"""
grafana docker
docker run -d -p 3000:3000 grafana/grafana
influxdb
step1 : docker pull influxdb
step2 :
docker run -p 8086:8086 -v $PROJECT_PATH/WhaleShark_IIoT/config:/var/lib/influxdb \
influxdb -config /var/lib/influxdb/influxdb.conf \
-e INFLUXDB_ADMIN_USER=whaleshark -e INFLUXDB_ADMIN_PASSWORD=whaleshark
Please refer https://www.open-plant.com/knowledge-base/how-to-install-influxdb-docker-for-windows-10/

Get connector for redis
If you don't have redis, you can use redis on docker with follow steps.
Getting most recent redis image
shell
docker pull redis
docker run --name whaleshark-redis -d -p 6379:6379 redis
docker run -it --link whaleshark-redis:redis --rm redis redis-cli -h redis -p 6379
if container is exist
 docker restart  whaleshark-redis
 
If you don't have rabbitmq, you can use docker.
docker run -d --hostname whaleshark --name whaleshark-rabbit -p 5672:5672 \
-p 8080:15672 -e RABBITMQ_DEFAULT_USER=whaleshark -e \
RABBITMQ_DEFAULT_PASS=whaleshark rabbitmq:3-management
if container is exist
 docker restart  whaleshark-rabbit
        
"""


class TcpServer:

    def __init__(self):
        with open('config/config_server_develop.yaml', 'r') as file:
            config_obj = yaml.load(file, Loader=yaml.FullLoader)
            self.tcp_host = config_obj['iiot_server']['tcp_server']['ip_address']
            self.tcp_port = config_obj['iiot_server']['tcp_server']['port']

            self.redis_host = config_obj['iiot_server']['redis_server']['ip_address']
            self.redis_port = config_obj['iiot_server']['redis_server']['port']

            self.rabbitmq_host = config_obj['iiot_server']['rabbit_mq']['ip_address']
            self.rabbitmq_port = config_obj['iiot_server']['rabbit_mq']['port']

            self.rabbitmq_id = config_obj['iiot_server']['rabbit_mq']['id']
            self.rabbitmq_pwd = config_obj['iiot_server']['rabbit_mq']['pwd']

            self.exchange = config_obj['iiot_server']['rabbit_mq']['exchange']
            self.exchange_type = config_obj['iiot_server']['rabbit_mq']['exchange_type']

    def connect_redis(self, host, port):
        '''
        :param host: redis access host ip
        :param port: redis access port
        :return: redis connector
        '''
        redis_obj = None
        try:
            conn_params = {
                "host": host,
                "port": port,
            }
            redis_obj = redis.StrictRedis(**conn_params)

        except Exception as e:
            logging.error(str(e))

        return redis_obj

    def config_equip_desc(self, address, port):
        '''
        Configure redis for equipment sensor desc(sensor_cd)
        key : const sensor_cd
        value : dictionary or map has sensor_cd:sensor description
        :return: redis connector
        '''
        redis_con = None
        try:
            redis_con = self.connect_redis(address, port)
            facilities_dict = redis_con.get('facilities_info')
            if facilities_dict is None:
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

        except Exception as e:
            logger.error(str(e))

        return redis_con

    def get_messagequeue(self, address, port):
        '''
        get message queue connector (rabbit mq) with address, port
        :param address: rabbit mq server ip
        :param port: rabbitmq server port(AMQP)
        :return: rabbitmq connection channel
        '''
        channel = None
        try:
            credentials = pika.PlainCredentials(self.rabbitmq_id, self.rabbitmq_pwd)
            param = pika.ConnectionParameters(address, port, '/', credentials)
            connection = pika.BlockingConnection(param)
            channel = connection.channel()
            channel.exchange_declare(exchange='facility', exchange_type=self.exchange_type)
        except Exception as e:
            logger.exception(str(e))

        return channel

    def init_config(self):
        self.redis_con = self.config_equip_desc(address=self.redis_host, port=self.redis_port)
        if self.redis_con is None:
            logger.error('redis configuration fail')
            sys.exit()

        self.mq_channel = self.get_messagequeue(address=self.rabbitmq_host, port=self.rabbitmq_port)
        if self.mq_channel is None:
            logger.error('rabbitmq configuration fail')
            sys.exit()

    def get_redis_con(self):
        return self.redis_con

    def get_mq_channel(self):
        return self.mq_channel

    def get_server_socket(self):
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_socket.setblocking(0)
        server_socket.bind(('', self.tcp_port))
        server_socket.listen(1)
        logging.debug('IIoT Client Ready ({ip}:{port})'.format(ip=self.tcp_host, port=self.tcp_port))
        self.redis_con.set('remote_log:iit_server_boot',json.dumps({'ip':self.tcp_host,'port':self.tcp_port, 'status':1}))
        return server_socket


if __name__ == '__main__':
    try:
        server = TcpServer()
        server.init_config()
        redis_mgr = server.get_redis_con()
        rabbit_channel = server.get_mq_channel()
        server_socket = server.get_server_socket()
        msg_size = 27
        async_server = AsyncServer(redis_mgr)
        event_manger = asyncio.get_event_loop()
        event_manger.run_until_complete(
            async_server.get_client(event_manger, server_socket, msg_size, rabbit_channel, server.exchange_type))

    except Exception as e:
        logger.error(str(e))
