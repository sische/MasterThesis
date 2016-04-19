import asyncio
from aiocoap import *
SERVER_ADDR = '2001::298:6bff:feef:d5f3'
SERVER_PORT = '5683'
SERVER_URI  = 'coap://[' + SERVER_ADDR + ']:' + SERVER_PORT
@asyncio.coroutine
def main():
	protocol = yield from Context.create_client_context()
	sequence_number = 1
#	number_of_simulations = 7
#	while sequence_number < number_of_simulations:
	while True:
		request_hrm = Message(code=GET)
		request_hrm.set_request_uri(SERVER_URI  + '/hrm')
		response_hrm = yield from protocol.request(request_hrm).response

#		request_led = Message(code=GET)
#		request_led.set_request_uri(SERVER_URI  + '/lights/led4')
#		response_led = yield from protocol.request(request_led).response

#		request_thermometer = Message(code=GET)
#		request_thermometer.set_request_uri(SERVER_URI  + '/thermometer')
#		response_thermometer = yield from protocol.request(request_thermometer).response

#		print(sequence_number)
#		print(response_thermometer.payload)
		if (response_hrm.payload != b'0'):		
			print('HRM value: %s Response Code: %s\n'%(response_hrm.payload, response_hrm.code))
#		sequence_number += 1
if __name__ == "__main__":
	asyncio.get_event_loop().run_until_complete(main())
