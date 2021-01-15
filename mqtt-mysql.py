import paho.mqtt.client as mqtt
import datetime
import time
import MySQLdb

MQTT_Broker = "localhost"
MQTT_Port = 1883
Keep_Alive_Interval = 60
MQTT_Topic = "#"

def on_connect(client, userdata, flags, rc):
	print("Connected")
	mq.subscribe(MQTT_Topic, 0)

def on_subscribe(mosq, obj, mid, granted_qos):
	print("Subscribed")
	pass

def on_message(client, userdata, msg):
	s = msg.topic+'\t'+msg.payload.decode("utf-8","ignore")
	s = s.replace("'",'"')
	st = datetime.datetime.fromtimestamp(time.time()).strftime('%Y-%m-%d %H:%M:%S.%f')
	print(s)
	db = MySQLdb.connect("localhost", "monitor", "password", "store")
	curs = db.cursor()
	#try:
	curs.execute ("INSERT INTO data (t,s) values ('"+st+"', '"+s+"');""")
	db.commit()
	#except:
	#        print("Error")
	#        db.rollback()

print("Start")
mq = mqtt.Client()
mq.enable_logger()
mq.on_message = on_message
mq.on_connect = on_connect
mq.on_subscribe = on_subscribe
print("Connecting...")
mq.connect(MQTT_Broker, port=1883, keepalive=60, bind_address="")

mq.loop_forever()

try:
        client.loop_forever()
except KeyboardInterrupt:
        client.disconnect()

