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
redis_con.close()
