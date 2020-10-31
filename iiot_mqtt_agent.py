import logging
import yaml
import json
import sys
from influxdb import InfluxDBClient
import time
import mongo_manager

from redis_manager import RedisMgr
from rabbit_mq_manager import RabbitMQMgr

logging.basicConfig(format='%(asctime)s %(levelname)-8s %(message)s',
                    stream=sys.stdout, level=logging.DEBUG, datefmt='%Y-%m-%d %H:%M:%S')
logging.getLogger("pika").propagate = False

"""
If you don't have rabbitmq, you can use docker.
docker run -d --hostname whaleshark --name whaleshark-rabbit \
-p 5672:5672 -p 8080:15672 -e RABBITMQ_DEFAULT_USER=whaleshark \
-e RABBITMQ_DEFAULT_PASS=whaleshark rabbitmq:3-management
"""

class Agent:
    def __init__(self):
        with open('config/config_server_develop.yaml', 'r') as file:
            config_obj = yaml.load(file, Loader=yaml.FullLoader)
        
            self.influx_host = config_obj['iiot_server']['influxdb']['ip_address']
            self.influx_port = config_obj['iiot_server']['influxdb']['port']
        
            self.influx_id = config_obj['iiot_server']['influxdb']['id']
            self.influx_pwd = config_obj['iiot_server']['influxdb']['pwd']
            self.influx_db = config_obj['iiot_server']['influxdb']['db']

        self.mongo_mgr = mongo_manager.MongoMgr()

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

    def callback_mqreceive(self, ch, method, properties, body):
        body = body.decode('utf-8')
        facility_msg_json = json.loads(body)
        table_name = list(facility_msg_json.keys())[0]
        fields = {}
        # tags = {}
        logging.debug('mqtt body:' + str(facility_msg_json))
        me_timestamp = time.time()
        for key in facility_msg_json[table_name].keys():
            if key != 'pub_time':
                logging.debug('config key:' + key + 'value:' + str(facility_msg_json[table_name][key]))
                fields[key] = float(facility_msg_json[table_name][key])
    
        pub_time = facility_msg_json[table_name]['pub_time']
        day = pub_time.split(' ')[0]
        pub_doc = self.mongo_mgr.document_bykey('facility', table_name, {'DAY': day})
        if pub_doc is not None:
            self.mongo_mgr.document_upsert('facility', table_name, day, pub_time, status='CHECK')
    
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

        self.mq_mgr = RabbitMQMgr()
        if self.mq_mgr.connect() is None:
            logging.error('rabbitmq configuration fail')
            
        self.redis_mgr = RedisMgr()
        if self.redis_mgr.connect() is None:
            logging.error('redis configuration fail')
    
    def get_influxdb_mgr(self):
        return self.influxdb_mgr
    
    def syncmessage(self):
        self.redis_mgr.config_equip_desc()
        facilities_dict = json.loads(self.redis_mgr.get('facilities_info'))
        for facility_id in facilities_dict.keys():
            result = self.mq_mgr.get_channel().queue_declare(queue=facility_id, exclusive=True)
            tx_queue = result.method.queue
            self.mq_mgr.get_channel().queue_bind(exchange='facility', queue=tx_queue)
            # call_back_arg = {'measurement': tx_queue}
            try:
                self.mq_mgr.get_channel().basic_consume(tx_queue, on_message_callback=self.callback_mqreceive)
            except Exception as exp:
                logging.error(str(exp))

        self.mq_mgr.get_channel().start_consuming()
        
if __name__ == '__main__':
    mqtt_agent = Agent()
    mqtt_agent.resource_config()
    mqtt_agent.syncmessage()