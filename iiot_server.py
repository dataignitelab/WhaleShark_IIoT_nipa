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
from logging import handlers

from net_socket.redis_init_info import init_facilities_info

logger = logging.getLogger(__name__)
logging.basicConfig(level=logging.DEBUG)
formatter = logging.Formatter('[%(asctime)s-%(name)s-%(levelname)s-%(filename)s:%(lineno)s-%(funcName)10s()]%(message)s')

logHandler = handlers.RotatingFileHandler('log/iiot_server_debug.log', maxBytes=1, backupCount=0)
logHandler.setLevel(logging.DEBUG)
logHandler.setFormatter(formatter)

errorLogHandler = handlers.RotatingFileHandler('log/iiot_server_error.log', maxBytes=1, backupCount=0)
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
            self.redis_pwd = config_obj['iiot_server']['redis_server']['pwd']

            self.rabbitmq_host = config_obj['iiot_server']['rabbit_mq']['ip_address']
            self.rabbitmq_port = config_obj['iiot_server']['rabbit_mq']['port']

            self.rabbitmq_id = config_obj['iiot_server']['rabbit_mq']['id']
            self.rabbitmq_pwd = config_obj['iiot_server']['rabbit_mq']['pwd']

            self.exchange = config_obj['iiot_server']['rabbit_mq']['exchange']
            self.exchange_type = config_obj['iiot_server']['rabbit_mq']['exchange_type']

    def connect_redis(self, host, port, pwd):
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
                'db': 0,
                "password": pwd
            }
            redis_obj = redis.StrictRedis(**conn_params)

        except Exception as e:
            logging.error(str(e))

        return redis_obj

    def config_equip_desc(self, address, port, pwd):
        '''
        Configure redis for equipment sensor desc(sensor_cd)
        key : const sensor_cd
        value : dictionary or map has sensor_cd:sensor description
        :return: redis connector
        '''
        redis_con = None
        try:
            redis_con = self.connect_redis(address, port, pwd)
            facilities_binary = redis_con.get('facilities_info')
            if facilities_binary is None:
                logger.debug('redis facilities_info reset')
                init_facilities_info(redis_con)

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
            logger.error(str(e))

        return channel

    def init_config(self):
        self.redis_con = self.config_equip_desc(address=self.redis_host, port=self.redis_port, pwd=self.redis_pwd)
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
        try:
            server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)  #프로세스 강제 종료 후 제시작시 소켓 재사용 가능하도록
            server_socket.setblocking(0)
            server_socket.bind(('', self.tcp_port))
            server_socket.listen(1)
            logging.debug('IIoT Client Ready ({ip}:{port})'.format(ip=self.tcp_host, port=self.tcp_port))
            self.redis_con.set('remote_log:iit_server_boot',json.dumps({'ip':self.tcp_host,'port':self.tcp_port, 'status':1}))
            return server_socket
        except Exception as e:
            logger.error(str(e))


if __name__ == '__main__':
    try:
        logger.debug('iiot_server active')
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
