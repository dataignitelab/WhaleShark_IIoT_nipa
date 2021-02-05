import sys
import json
from iiot_server import TcpServer

server = TcpServer()
server.redis_con = server.config_equip_desc(address='localhost', port=6379)
if server.redis_con is None:
    sys.exit()
redis_con = server.get_redis_con()

facilities_dict = redis_con.get('facilities_info')
facilities_json = json.loads(facilities_dict)
print(facilities_json)
facilities_dict = {
    'TS0001':
        {
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
    'TS0002':
        {
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
    'TS0003':
        {
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
    'TS0004':
        {
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
    'TS0005':
        {
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
    'TS0006':
        {
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
    'TS0007':
        {
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
    'TS0008':
        {
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
    'TS0009':
        {
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
    'TS9999':
        {
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
    }
redis_con.set('facilities_info', json.dumps(facilities_dict))
redis_con.close()
