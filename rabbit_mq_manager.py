import pika
import yaml
import sys
import json
import logging

logging.basicConfig(format='%(asctime)s %(levelname)-8s %(message)s',stream=sys.stdout, level=logging.DEBUG, datefmt='%Y-%m-%d %H:%M:%S')
logging.getLogger("pika").propagate = False

"""
If you don't have rabbitmq, you can use docker.
docker run -d --hostname whaleshark --name whaleshark-rabbit -p 5672:5672 \
-p 8080:15672 -e RABBITMQ_DEFAULT_USER=whaleshark -e \
RABBITMQ_DEFAULT_PASS=whaleshark rabbitmq:3-management
"""

class RabbitMQMgr:

    def __init__(self):
        self.redis_con = None
        self.rabbitmq_host = None
        self.rabbitmq_port = None
        self.rabbitmq_id = None
        self.rabbitmq_pwd = None
        self.exchange = None
        self.exchange_type = None
        with open('config/config_server_develop.yaml', 'r') as file:
            config_obj = yaml.load(file, Loader=yaml.FullLoader)
            self.rabbitmq_host = config_obj['iiot_server']['rabbit_mq']['ip_address']
            self.rabbitmq_port = config_obj['iiot_server']['rabbit_mq']['port']

            self.rabbitmq_id = config_obj['iiot_server']['rabbit_mq']['id']
            self.rabbitmq_pwd = config_obj['iiot_server']['rabbit_mq']['pwd']

            self.exchange = config_obj['iiot_server']['rabbit_mq']['exchange']
            self.exchange_type = config_obj['iiot_server']['rabbit_mq']['exchange_type']


    def connect(self):
        '''
        get message queue connector (rabbit mq) with address, port
        :param address: rabbit mq server ip
        :param port: rabbitmq server port(AMQP)
        :return: rabbitmq connection channel
        '''
        self.channel = None
        try:
            credentials = pika.PlainCredentials(self.rabbitmq_id, self.rabbitmq_pwd)
            param = pika.ConnectionParameters(self.rabbitmq_host, self.rabbitmq_port, '/', credentials)
            connection = pika.BlockingConnection(param)
            self.channel = connection.channel()
            self.channel.exchange_declare(exchange=self.exchange, exchange_type=self.exchange_type)
        except Exception as e:
            logging.exception(str(e))

        return self.channel

    def get_channel(self):
        return self.channel

