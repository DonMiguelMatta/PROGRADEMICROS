import sys
import time

import serial
from Adafruit_IO import Client
from Adafruit_IO.errors import RequestError


ADAFRUIT_IO_USERNAME = "Don22993"
ADAFRUIT_IO_KEY = "pegar"

SERIAL_PORT = "COM3"
BAUD_RATE = 9600

POT_FEEDS = ["pot-1", "pot-2", "pot-3", "pot-4"]
LED_FEED = "led1-rx"
SEND_TIME = 10
LED_READ_TIME = 2

last_send_time = 0
last_led_time = 0
last_led_value = ""


def open_serial():
	try:
		arduino = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
		print("Arduino conectado en", SERIAL_PORT)
		return arduino
	except serial.SerialException as error:
		print("No se pudo abrir", SERIAL_PORT)
		print("Revisa que el Monitor Serial este cerrado y que el COM sea correcto.")
		print("Error:", error)
		sys.exit(1)


def connect_adafruit():
	try:
		aio = Client(ADAFRUIT_IO_USERNAME, ADAFRUIT_IO_KEY)
		aio.feeds()
		print("Conectado a Adafruit IO")
		return aio
	except Exception as error:
		print("No se pudo conectar a Adafruit IO.")
		print("Revisa usuario y AIO Key.")
		print("Error:", error)
		sys.exit(1)


def parse_pots(line):
	if not line.startswith("P:"):
		return None

	values = line[2:].split(",")

	if len(values) != 4:
		return None

	try:
		values = [int(value) for value in values]
	except ValueError:
		return None

	return [max(0, min(1023, value)) for value in values]


def send_pots(aio, values):
	for feed, value in zip(POT_FEEDS, values):
		percent = round((value * 100) / 1023)
		print("Enviando", feed, percent)
		aio.send_data(feed, percent)
		time.sleep(0.5)


def read_led(aio, arduino):
	global last_led_value

	try:
		data = aio.receive(LED_FEED)
	except RequestError:
		print("No se pudo leer", LED_FEED)
		return

	value = str(data.value).upper()

	if value == last_led_value:
		return

	last_led_value = value
	print("LED desde Adafruit:", value)

	if value == "ON":
		arduino.write(b"O")
	else:
		arduino.write(b"F")


arduino = open_serial()
aio = connect_adafruit()

while True:
	raw_line = arduino.readline()

	if raw_line:
		line = raw_line.decode("ascii", errors="ignore").strip()
		print("Arduino:", line)

		pots = parse_pots(line)

		if pots is not None and (time.time() - last_send_time) >= SEND_TIME:
			send_pots(aio, pots)
			last_send_time = time.time()

	if (time.time() - last_led_time) >= LED_READ_TIME:
		read_led(aio, arduino)
		last_led_time = time.time()

	time.sleep(0.05)
