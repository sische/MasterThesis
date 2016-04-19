from flask import Flask

app = Flask(__name__)

@app.route('/')
#@app.route('/home', methods=['POST'])
def ajaxReply():
	#receivedNumber = responseList[0]
	return "Hello World 2!"	
	#return jsonify(id='accel', number=7) #number=reveivedNumber)	


if __name__ == '__main__':
	app.run()
