import logging
import yaml
import json
import pika
import sys
import redis
from influxdb import InfluxDBClient
import time

logging.basicConfig(format='%(asctime)s %(levelname)-8s %(message)s',
                    stream=sys.stdout, level=logging.DEBUG, datefmt='%Y-%m-%d %H:%M:%S')
logging.getLogger("pika").propagate = False

class Agent:
    def __init__(self):
        with open('config/config_server_develop.yaml', 'r') as file:
            config_obj = yaml.load(file, Loader=yaml.FullLoader)
            self.rabbitmq_host = config_obj['iiot_server']['rabbit_mq']['ip_address']
            self.rabbitmq_port = config_obj['iiot_server']['rabbit_mq']['port']
       
            self.redis_host = config_obj['iiot_server']['redis_server']['ip_address']
            self.redis_port = config_obj['iiot_server']['redis_server']['port']
        
            self.influx_host = config_obj['iiot_server']['influxdb']['ip_address']
            self.influx_port = config_obj['iiot_server']['influxdb']['port']
        
            self.influx_id = config_obj['iiot_server']['influxdb']['id']
            self.influx_pwd = config_obj['iiot_server']['influxdb']['pwd']
            self.influx_db = config_obj['iiot_server']['influxdb']['db']


    def connect_redis(self, host, port):
        """
        Get connector for redis
        If you don't have redis, you can use redis on docker with follow steps.
        Getting most recent redis image
        shell: docker pull redis

        docker pull redis
        docker run --name whaleshark-redis -d -p 6379:6379 redis
        docker run -it --link whaleshark-redis:redis --rm redis redis-cli -h redis -p 6379

        :param host: redis access host ip
        :param port: redis access port
        :return: redis connector
        """
        redis_obj = None
        try:
            conn_params = {
                "host": host,
                "port": port,
                "db":0
            }
            redis_obj = redis.StrictRedis(**conn_params)
        except Exception as exp:
            logging.error(str(exp))
        return redis_obj

    def get_influxdb(self, host, port, name, pwd, db):
        """
        :param host: InfluxDB access host ip
        :param port: InfluxDB access port
        :param name: InfluxDB access user name
        :param pwd: InfluxDB access user password
        :param db: Database to access
        :return: InfluxDB connector
        """
        client = None
        try:
            client = InfluxDBClient(host=host, port=port, username=name, password=pwd, database=db)
        except Exception as exp:
            logging.error(str(exp))
        return client

    def get_messagequeue(self, address, port):
        """
        If you don't have rabbitmq, you can use docker.
        docker run -d --hostname whaleshark --name whaleshark-rabbit \
        -p 5672:5672 -p 8080:15672 -e RABBITMQ_DEFAULT_USER=whaleshark \
        -e RABBITMQ_DEFAULT_PASS=whaleshark rabbitmq:3-management

        get message queue connector (rabbit mq) with address, port
        :param address: rabbit mq server ip
        :param port: rabbitmq server port(AMQP)
        :return: rabbitmq connection channel
        """
        channel = None
        try:
            credentials = pika.PlainCredentials('whaleshark', 'whaleshark')
            param = pika.ConnectionParameters(address, port, '/', credentials)
            connection = pika.BlockingConnection(param)
            channel = connection.channel()
    
        except Exception as exp:
            logging.exception(str(exp))
    
        return channel

    def callback_mqreceive(self, ch, method, properties, body):
        body = body.decode('utf-8')
        facility_msg_json = json.loads(body)
        table_name = list(facility_msg_json.keys())[0]
        fields = {}
        tags = {}
        logging.debug('mqtt body:' + str(facility_msg_json))
        me_timestamp = time.time()
        for key in facility_msg_json[table_name].keys():
            if key != 'pub_time':
                logging.debug('config key:' + key + 'value:' + str(facility_msg_json[table_name][key]))
                fields[key] = float(facility_msg_json[table_name][key])
    
        pub_time = facility_msg_json[table_name]['pub_time']

        fields['me_time'] = me_timestamp
        influx_json = [{
            'measurement': table_name,
            'fields': fields
        }]
        try:
            if self.influxdb_mgr.write_points(influx_json) is True:
                logging.debug('influx write success:' + str(influx_json))
            else:
                logging.debug('influx write faile:' + str(influx_json))
        except Exception as exp:
            print(str(exp))

    def resource_config(self):
        self.influxdb_mgr = self.get_influxdb(host=self.influx_host, port=self.influx_port, name=self.influx_id, pwd=self.influx_pwd, db=self.influx_db)
        if self.influxdb_mgr is None:
            logging.error('influxdb configuration fail')

        self.mq_channel = self.get_messagequeue(address=self.rabbitmq_host, port=self.rabbitmq_port)
        if self.mq_channel is None:
                logging.error('rabbitmq configuration fail')
            
        self.redis_mgr = self.connect_redis(self.redis_host, self.redis_port)
    
    def get_influxdb_mgr(self):
        return self.influxdb_mgr
    
    def syncmessage(self):
        devinfo = json.loads(self.redis_mgr.get('dev_info'))
        for dev_id in devinfo.keys():
            result = self.mq_channel.queue_declare(queue=dev_id, exclusive=True)
            tx_queue = result.method.queue
            self.mq_channel.queue_bind(exchange='facility', queue=tx_queue)
            call_back_arg = {'measurement': tx_queue}
            try:
                self.mq_channel.basic_consume(tx_queue, on_message_callback=self.callback_mqreceive)
            except Exception as exp:
                logging.error(str(exp))

        self.mq_channel.start_consuming()
        
if __name__ == '__main__':
    mqtt_agent = Agent()
    mqtt_agent.resource_config()
    mqtt_agent.syncmessage()