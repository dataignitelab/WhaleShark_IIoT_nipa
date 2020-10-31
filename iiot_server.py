import asyncio
import socket
import logging
import yaml
import sys
import json
from redis_manager import RedisMgr
from rabbit_mq_manager import RabbitMQMgr
from net_socket.iiot_tcp_async_server import AsyncServer

logging.basicConfig(format='%(asctime)s %(levelname)-8s %(message)s',stream=sys.stdout, level=logging.DEBUG, datefmt='%Y-%m-%d %H:%M:%S')
logging.getLogger("pika").propagate = False
"""
grafana docker
docker run -d -p 3000:3000 grafana/grafana
influxdb
step1 : docker pull influxdb
step2 :
docker run -p 8086:8086 -v $PROJECT_PATH/WhaleShark_IIoT/config:/var/lib/influxdb \
-e INFLUXDB_ADMIN_USER=whaleshark -e INFLUXDB_ADMIN_PASSWORD=whaleshark\
influxdb -config /var/lib/influxdb/influxdb.conf
Please refer https://www.open-plant.com/knowledge-base/how-to-install-influxdb-docker-for-windows-10/

Get connector for redis
If you don't have redis, you can use redis on docker with follow steps.
Getting most recent redis image
shell
docker pull redis
docker run --name whaleshark-redis -d -p 6379:6379 redis
docker run -it --link whaleshark-redis:redis --rm redis redis-cli -h redis -p 6379

If you don't have rabbitmq, you can use docker.
docker run -d --hostname whaleshark --name whaleshark-rabbit -p 5672:5672 \
-p 8080:15672 -e RABBITMQ_DEFAULT_USER=whaleshark -e \
RABBITMQ_DEFAULT_PASS=whaleshark rabbitmq:3-management
        
"""


class TcpServer:

    def __init__(self):
        with open('config/config_server_develop.yaml', 'r') as file:
            config_obj = yaml.load(file, Loader=yaml.FullLoader)
            self.tcp_host = config_obj['iiot_server']['tcp_server']['ip_address']
            self.tcp_port = config_obj['iiot_server']['tcp_server']['port']

    def init_config(self):
        self.redis_mgr = RedisMgr()
        if self.redis_mgr.connect() is None:
            logging.error('redis configuration fail')
            sys.exit()
        self.redis_mgr.config_equip_desc()

        self.mq_mgr = RabbitMQMgr()
        if self.mq_mgr.connect() is None:
            logging.error('rabbitmq configuration fail')
            sys.exit()

    def get_redis_conn(self):
        return self.redis_mgr.get_conn()

    def get_mq_channel(self):
        return self.mq_mgr.get_channel()

    def get_server_socket(self):
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_socket.setblocking(0)
        server_socket.bind(('', self.tcp_port))
        server_socket.listen(1)
        logging.debug('IIoT Client Ready ({ip}:{port})'.format(ip=self.tcp_host, port=self.tcp_port))
        self.redis_mgr.set('remote_log:iit_server_boot',json.dumps({'ip':self.tcp_host,'port':self.tcp_port, 'status':1}))
        return server_socket


if __name__ == '__main__':
    try:
        server = TcpServer()
        server.init_config()

        redis_mgr = server.get_redis_conn()
        rabbit_channel = server.get_mq_channel()
        server_socket = server.get_server_socket()
        msg_size = 27
        async_server = AsyncServer(redis_mgr)
        event_manger = asyncio.get_event_loop()
        event_manger.run_until_complete(
            async_server.get_client(event_manger, server_socket, msg_size, rabbit_channel))

    except Exception as e:
        print(str(e))
