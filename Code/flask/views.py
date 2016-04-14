"""
Routes and views for the flask application.
"""
import asyncio
from aiocoap import *
import matplotlib.pyplot as plt

import sys, re, os, random
from datetime import datetime
from flask import render_template, request, session, jsonify
from flask.ext.session import Session
from FlaskWebProject import app
# __file__ refers to the file settings.py 
APP_ROOT = os.path.dirname(os.path.abspath(__file__))   # refers to application_top
APP_STATIC = os.path.join(APP_ROOT, 'static')

SERVER_ADDR = '2001::2af:b7ff:feb6:1494'
SERVER_PORT = '5683'
SERVER_URI  = 'coap://[' + SERVER_ADDR + ']:' + SERVER_PORT

responseList = []
receivedNumber = 0


app.secret_key = os.urandom(24)
app.config['SESSION_TYPE'] = 'filesystem'
Session(app)

#app.debug = True



def observe_handle(response):
	#f = open('/home/sindre/Desktop/desktopAccelValues', 'a')
	if response.code.is_successful(): # and i < 5):

		responseList = bytes.decode(response.payload) # bytes.decode, split()	
		print(responseList)


		#for i in range(0, (len(responseList))):
			#print("Length of list:" + str(len(responseList)))
		#	f.write((str(responseList[i]) + ' '))
		#f.write('\n')
		#print("Written to file!")
		#plt.plot([1,2,3,4])
		#plt.ylabel('some numbers')
		#plt.show()
	else:
		print('Error code %s' % response.code)
	#f.close()
@asyncio.coroutine


@app.route('/home', methods=['POST'])
def ajaxReply(title):
	"""Renders the questions page and checks committed answer."""
	#sess = session['nofCorrect'].split(',')
	#count = int(sess[0].replace('correct=', ''))
	receivedNumber = responseList[0]	
	
	return jsonify(id='accel', number=reveivedNumber)	


	'''try:
		questions = getQuestions(title)
		formatted = formatQuestions(questions)
	except IOError:
		message = 'No questions found'
		print("error reading file")
	questionDict = formatted[int(request.form.get('question'))]
	if (questionDict['correct'] == request.form.get('answer')):
		if request.form.get('question') not in sess:
			session['nofCorrect'] += ',' + request.form.get('question')
			count += 1
			session['nofCorrect'] = re.sub(r'correct=[0-9]+','correct=' + str(count),session['nofCorrect'])
			#session['nofCorrect'] = int(session['nofCorrect']) + 1
		return jsonify(correct=True, id=request.form.get('question'), nofCorrect=count)
	else:
		if request.form.get('question') not in sess:
			session['nofCorrect'] += ',' + request.form.get('question')
		#session['nofCorrect'] = int(session['nofCorrect']) - 1
		return jsonify(correct=False, id=request.form.get('question'), nofCorrect=count)
	'''

	
def getQuestions(theme):
	#fix add a settingsfile to hold filenames etc, atleast something else than theme.lower() + _questions.txt
	filename = os.path.join(APP_STATIC, 'files', (theme.lower() + '_questions.txt'))
	questions = []
	with open(filename, 'r') as f:
		for line in f:
			line = line.replace('\n', '')
			questions.append(line)
	return questions

def formatQuestions(questions):
	return_questions = []
	qid = 0
	for question in questions:
		form = {}
		form['id'] = qid
		elementList = question.split('	')
		form['question'] = elementList.pop(0)
		correctAlt = elementList.pop(-1)
		if (correctAlt == 'a'):
			form['correct'] = elementList[0]
		elif (correctAlt == 'b'):
			form['correct'] = elementList[1]
		elif (correctAlt == 'c'):
			form['correct'] = elementList[2]
		elif (correctAlt == 'd'):
			form['correct'] = elementList[3]
		elif (correctAlt == 'e'):
			form['correct'] = elementList[4]
		form['answers'] = sorted(elementList, key=lambda k: random.random())
		qid = qid + 1
		return_questions.append(form)
	return return_questions

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

