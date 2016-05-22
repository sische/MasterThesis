import asyncio
from aiocoap import *
import matplotlib.pyplot as plt

#SERVER_ADDR = '2001::211:64ff:fea5:8542'
SERVER_ADDR = '2001::2e6:6aff:fe64:54dd'
#SERVER_ADDR = '2001::2af:b7ff:feb6:1494'

SERVER_PORT = '5683'
SERVER_URI  = 'coap://[' + SERVER_ADDR + ']:' + SERVER_PORT

responseList = []
drawValuesList = []

def observe_handle(response):
	if response.code.is_successful(): 	
		responseList = bytes.decode(response.payload) 	
		print(responseList)

		for i in range (0,len(responseList)):
			drawValuesListappend(int(responseList[i]))
		plt.plot(drawValuesList)
		plt.xlabel('Measurement number')
		plt.ylabel('Acceleration values')
		plt.show()	
		
	else:
		print('Error code %s' % response.code)
@asyncio.coroutine

def main():
    protocol = yield from Context.create_client_context()
    request = Message(code=GET)
    request.set_request_uri(SERVER_URI  + '/lights/led3')
    request.opt.observe = 0
    observation_is_over = asyncio.Future()
    try:
        requester = protocol.request(request)
        requester.observation.register_callback(observe_handle)
        response = yield from requester.response
        exit_reason = yield from observation_is_over
        print('Observation is over: %r' % exit_reason)
    finally:
        if not requester.response.done():
            requester.response.cancel()
        if not requester.observation.cancelled:
            requester.observation.cancel()

if __name__ == "__main__":
    asyncio.get_event_loop().run_until_complete(main())

