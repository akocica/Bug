import paho.mqtt.client as mqtt
import datetime
import sqlite3 as lite
from time import time_ns

MQTT_Broker = "localhost"
MQTT_Port = 1883
Keep_Alive_Interval = 60
MQTT_Topic = "bug"



def on_connect(client, userdata, flags, rc):
	print("Connected")
	mq.subscribe(MQTT_Topic, 0)

def on_subscribe(mosq, obj, mid, granted_qos):
	print("Subscribed")
	pass

def on_message(client, userdata, msg):
	s = msg.payload.decode("utf-8","ignore")
	cur.execute("INSERT INTO "+table+" VALUES(?, ?)", (time_ns(), s))
	con.commit()

print("Start")
table = datetime.datetime.now().strftime("data%Y%m%d%H%M%S")
con = lite.connect('bug.db')
cur = con.cursor()
cur.execute("CREATE TABLE "+table+"(id INT, txt TEXT)")
    		
mq = mqtt.Client()
mq.enable_logger()

mq.on_message = on_message
mq.on_connect = on_connect
mq.on_subscribe = on_subscribe

print("Connecting...")
mq.connect(MQTT_Broker, port=1883, keepalive=60, bind_address="")
mq.loop_forever()
#try:
client.loop_forever()
#except KeyboardInterrupt:
client.disconnect()

