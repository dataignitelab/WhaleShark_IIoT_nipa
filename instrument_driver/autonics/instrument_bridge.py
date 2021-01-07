import minimalmodbus
import serial


class instruments:

	def __init__(self):
		pass


	def connect(self,slave_desc):
		if slave_desc['com_type'] == 'serial':
			self.com_type = 'serial'
			self.instrument=minimalmodbus.Instrument(slave_desc['port'], slave_desc['stationid'], slave_desc['mode'])
			if slave_desc['parity'] == 'None':
				self.instrument.serial.parity=serial.PARITY_NONE
			elif slave_desc['parity'] == 'Even':
				self.instrument.serial.parity=serial.PARITY_EVEN
			elif slave_desc['parity'] == 'Odd':
				self.instrument.serial.parity=serial.PARITY_ODD

			self.instrument.serial.baudrate=slave_desc['baudrate']
			self.instrument.serial.bytesize=slave_desc['databits']
			self.instrument.serial.stopbits=slave_desc['stopbits']
			self.instrument.serial.timeout=slave_desc['timeout']
			self.instrument.mode=slave_desc['mode']
			self.function={}
			self.function['PV']=slave_desc['PV']
			self.function['SV']=slave_desc['SV']
			self.facility_name=(slave_desc['facility_name'][0:1],slave_desc['facility_name'][1:2])
			self.facility_id=(slave_desc['facility_id'][0:1],slave_desc['facility_id'][1:2])


	def get_function_value(self, address):
		"""
		get pv value of pid controller (model name : AUTONICS tk4s)
		"""
		try:
			fv = self.instrument.read_register(address, 0, functioncode=int('0x04', 16))
			return fv

		except Exception as e:
			print(e)


