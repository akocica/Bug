import paho.mqtt.client as mqtt
import datetime
import sqlite3 as lite
from time import time_ns

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
	s = msg.topic+'\t'+msg.payload.decode("utf-8","ignore").replace("'",'"')
	print(s)
	con = lite.connect('mqtt.db')
	with con:
    		cur = con.cursor()
    		cur.execute("DROP TABLE IF EXISTS data")
    		cur.execute("CREATE TABLE data(id INT, txt TEXT)")
    		cur.execute("INSERT INTO data VALUES(?, ?)", (time_ns(), s))

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

