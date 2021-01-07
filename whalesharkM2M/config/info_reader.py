import os
import pandas as pd

def read_controller(filepath):
    """
    Return dataframe from csv file
    """
    try:
        controller_df = pd.read_csv(filepath)
        return controller_df
    except Exception as e:
        print(e)
        
def read_deviceinfo(config_path):
    """
    Return information dictionary which has device info (controllerinfo.csv)
    """
    try:
        base_path = os.getcwd()
        ctrl_info_path = config_path + '/controllerinfo.csv'
        print('read dev info from:'+ctrl_info_path)
        ci_df = pd.read_csv(ctrl_info_path)
        device_info = {}
        dev_list = ci_df.DEVNAME.unique().tolist()
        for dev_name in dev_list:
            code_list = ci_df.loc[ci_df['DEVNAME'] == dev_name].SENSOR_CODE.unique().tolist()
            device_info[dev_name] = {}
            for code_name in code_list:
                sensor_name = ci_df.query(
                    "DEVNAME == '" + dev_name + "' and SENSOR_CODE == " + str(code_name)).SENSOR_NAME.tolist()
                print(dev_name, '%04d' % (code_name), sensor_name)
                if len(device_info.keys()) == 0:
                    device_info[dev_name] = {'%04d' % (code_name): sensor_name[0]}
                else:
                    device_info[dev_name]['%04d' % (code_name)] = sensor_name[0]
        return device_info
    except Exception as e:
        print(e)