import logging
import yaml
import json
import pika
import sys
import redis
from influxdb import InfluxDBClient
import time
# import mongo_manager

import logging
import logging.handlers as handlers

logger = logging.getLogger('iiot_mqtt_agent')
# FORMAT = "[%(filename)s:%(lineno)s - %(funcName)20s() ] %(message)s"
# logging.basicConfig(format=FORMAT)
# logger.setLevel(logging.DEBUG)

## Here we define our formatter
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(filename)s:%(lineno)s - %(funcName)20s() %(message)s')

logHandler = handlers.TimedRotatingFileHandler('log/iiot_mqtt_agent_debug.log', when='M', interval=1, backupCount=0)
logHandler.setLevel(logging.DEBUG)
logHandler.setFormatter(formatter)

errorLogHandler = handlers.RotatingFileHandler('log/iiot_mqtt_agent_error.log', maxBytes=5000, backupCount=0)
errorLogHandler.setLevel(logging.ERROR)
errorLogHandler.setFormatter(formatter)

logger.addHandler(logHandler)
logger.addHandler(errorLogHandler)

class Agent:
    def __init__(self):
        with open('config/config_server_develop.yaml', 'r') as file:
            config_obj = yaml.load(file, Loader=yaml.FullLoader)
            self.rabbitmq_host = config_obj['iiot_server']['rabbit_mq']['ip_address']
            self.rabbitmq_port = config_obj['iiot_server']['rabbit_mq']['port']
       
            self.redis_host = config_obj['iiot_server']['redis_server']['ip_address']
            self.redis_port = config_obj['iiot_server']['redis_server']['port']
            self.redis_pwd = config_obj['iiot_server']['redis_server']['pwd']

            self.influx_host = config_obj['iiot_server']['influxdb']['ip_address']
            self.influx_port = config_obj['iiot_server']['influxdb']['port']
        
            self.influx_id = config_obj['iiot_server']['influxdb']['id']
            self.influx_pwd = config_obj['iiot_server']['influxdb']['pwd']
            self.influx_db = config_obj['iiot_server']['influxdb']['db']

        # self.mongo_mgr = mongo_manager.MongoMgr()
    
    def connect_redis(self, host, port, pwd):
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
                'db': 0,
                "password": pwd
            }
            redis_obj = redis.StrictRedis(**conn_params)
        except Exception as exp:
            logger.error(str(exp))
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
            logger.error(str(exp))
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
            logger.exception(str(exp))
    
        return channel

    def callback_mqreceive(self, ch, method, properties, body):
        body = body.decode('utf-8')
        try:
            facility_msg_json = json.loads(body)
            logging.debug('mqreceice:%s' % (facility_msg_json))
            table_name = list(facility_msg_json.keys())[0]
            fields = {}
            tags = {}
            me_timestamp = time.time()
            for key in facility_msg_json[table_name].keys():
                if key != 'pub_time':
                    logging.debug('config key:' + key + 'value:' + str(facility_msg_json[table_name][key]))
                    fields[key] = float(facility_msg_json[table_name][key])

            pub_time = facility_msg_json[table_name]['pub_time']
            # day = pub_time.split(' ')[0]
            # pub_doc = self.mongo_mgr.document_bykey('facility', table_name, {'DAY': day})
            # if pub_doc is not None:
            #     self.mongo_mgr.document_upsert('facility', table_name, day, pub_time, status='CHECK')
            # else:
            #     logging.debug('Mongo exception facility:'+table_name+':DAY'+ str(day)+' NO EXIST')
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
                logger.error(str(exp))

        except Exception as e:
            logger.error('callback_mqreceive exception')
            logger.error(str(e))

    def config_facility_desc(self, redis_con):
        facilities_dict = redis_con.get('facilities_info')
        if facilities_dict is None:
            logging.debug('Redis key is non->reset')
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
            
    def resource_config(self):
        self.influxdb_mgr = self.get_influxdb(host=self.influx_host, port=self.influx_port, name=self.influx_id, pwd=self.influx_pwd, db=self.influx_db)
        if self.influxdb_mgr is None:
            logging.error('influxdb configuration fail')

        self.mq_channel = self.get_messagequeue(address=self.rabbitmq_host, port=self.rabbitmq_port)
        if self.mq_channel is None:
                logging.error('rabbitmq configuration fail')
            
        self.redis_mgr = self.connect_redis(self.redis_host, self.redis_port, self.redis_pwd)
    
    def get_influxdb_mgr(self):
        return self.influxdb_mgr
    
    def syncmessage(self):
        self.config_facility_desc(self.redis_mgr)
        facilities_dict = json.loads(self.redis_mgr.get('facilities_info'))
        for facility_id in facilities_dict.keys():
            result = self.mq_channel.queue_declare(queue=facility_id)#, exclusive=True)
            tx_queue = result.method.queue
            logging.debug('Queue bind exchange: %s queue %s'%('facility', facility_id))
            self.mq_channel.queue_bind(exchange='facility', queue=tx_queue)
            call_back_arg = {'measurement': tx_queue}
            try:
                self.mq_channel.basic_consume(tx_queue, on_message_callback=self.callback_mqreceive, auto_ack=True)
            except Exception as exp:
                logger.error(str(exp))

        self.mq_channel.start_consuming()
        
if __name__ == '__main__':
    mqtt_agent = Agent()
    mqtt_agent.resource_config()
    mqtt_agent.syncmessage()