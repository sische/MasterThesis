import asyncio
from aiocoap import *
SERVER_ADDR = '2001::2AF:B7FF:FEB6:1494'
SERVER_PORT = '5683'
SERVER_URI  = 'coap://[' + SERVER_ADDR + ']:' + SERVER_PORT
@asyncio.coroutine
def main():
	protocol = yield from Context.create_client_context()
	sequence_number = 1
	number_of_measurements = 200
	while sequence_number < number_of_measurements:
		request_acceleration = Message(code=GET)
		request_acceleration.set_request_uri(SERVER_URI + '/lights/led3')
		response = yield from protocol.request(request_acceleration).response
		print('Acceleration'+str(sequence_number)+': %s Response Code: %s\n'%(response.payload, response.code))		
		sequence_number += 1

if __name__ == "__main__":
	asyncio.get_event_loop().run_until_complete(main())
