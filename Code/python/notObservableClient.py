import asyncio
from aiocoap import *
#SERVER_ADDR = '2001::211:64ff:fea5:8542'
SERVER_ADDR = '2001::2e6:6aff:fe64:54dd'
SERVER_PORT = '5683'
SERVER_URI  = 'coap://[' + SERVER_ADDR + ']:' + SERVER_PORT
@asyncio.coroutine
def main():
    protocol = yield from Context.create_client_context()
    sequence_number = 1
    while sequence_number < 101:
      request_led3 = Message(code=GET)
      request_led3.set_request_uri(SERVER_URI  + '/lights/led3')
      #request_led4 = Message(code=GET)
      #request_led4.set_request_uri(SERVER_URI  + '/lights/led4')
      response = yield from protocol.request(request_led3).response
      print(response.payload)
      #response = yield from protocol.request(request_led4).response
      #print('State of LED 4: %s Response Code: %s\n'%(response.payload, response.code))
      #print('Sending PUT request to change both leds state\n')
      #request_led3 = Message(code=PUT, payload=b'1' if sequence_number % 2 else b'0')
      #request_led3.set_request_uri(SERVER_URI  + '/lights/led3')
      #request_led4 = Message(code=PUT, payload=b'0' if sequence_number % 2 else b'1')
      #request_led4.set_request_uri(SERVER_URI  + '/lights/led4')
      #response = yield from protocol.request(request_led3).response
      #print('Server Response Code: %s\n'%(response.code))
      #response = yield from protocol.request(request_led4).response
      #print('Server Response Code: %s\n'%(response.code))
      #sequence_number += 1
if __name__ == "__main__":
    asyncio.get_event_loop().run_until_complete(main())

