import asyncio
from aiocoap import *


#SERVER_ADDR = '2001::211:64ff:fea5:8542'
#SERVER_ADDR = '2001::2e6:6aff:fe64:54dd'
SERVER_ADDR = '2001::2af:b7ff:feb6:1494'
SERVER_PORT = '5683'
SERVER_URI  = 'coap://[' + SERVER_ADDR + ']:' + SERVER_PORT

responseList = []
drawValuesList = [0,0,0,0,0]


def observe_handle(response):
	#f = open('/home/sindre/Desktop/desktopAccelValues', 'a')
	if response.code.is_successful(): # and i < 5):
		#print('Current acceleration: %s Response Code: %s'%(response.payload, response.code))
		#str(response.payload).replace('b', '')		
		responseList = bytes.decode(response.payload) # bytes.decode, split()
		#responseList[0].replace("b", "")
		#for i in range(len(responseList)):			
		#	print ((responseList[i]))		
		print(responseList)
		#print(int(responseList[0]))
		#print(int(responseList[1]))


		#for i in range(0, (len(responseList))):
			#print("Length of list:" + str(len(responseList)))
		#	f.write((str(responseList[i]) + ' '))
		#f.write('\n')
		#print("Written to file!")
		#plt.plot([1,2,3,4])
		#plt.ylabel('some numbers')
		#plt.show()
		#drawValuesList[0] = (int(responseList[1]))
		#drawValuesList[1] = (int(responseList[2]))
		#drawValuesList[2] = (int(responseList[3]))
		#drawValuesList[3] = (int(responseList[4]))
		#drawValuesList[4] = (int(responseList[5]))
		#print(drawValuesList)
		#f.write(responseList[1] + ' ')
		#f.write(responseList[2] + ' ')
		#f.write(responseList[3] + ' ')
		#f.write(responseList[4] + ' ')
		#f.write(responseList[5] + '\n')
		#print("Written measurement to file!")
		#plt.plot(drawValuesList)
		#plt.ylabel('Acceleration values')
		#plt.show()	
		
	else:
		print('Error code %s' % response.code)
	#f.close()
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

