import RPi.GPIO as gpio
from time import sleep
import os


user = os.listdir('/home')[0]
gcode_console = f"/home/{user}/printer_data/comms/klippy.serial"


def run_gcode(command):
    with open(gcode_console, 'a') as console:
        console.write(command + '\n')


def restart_klipper(x):
    for _ in range(50):
        if gpio.input(12) == gpio.HIGH:
            return
        sleep(0.01)
    run_gcode("_OFF")
    sleep(1)
    run_gcode("M112")
    sleep(0.2)
    run_gcode("firmware_restart")


def shutdown_printer(x):
    for _ in range(50):
        if gpio.input(20) == gpio.LOW:
            return
        sleep(0.01)
    run_gcode("_SHUTDOWN")
    sleep(120)
    os.system("shutdown -h now")
    sleep(120)


gpio.setmode(gpio.BCM)

gpio.setup(16, gpio.OUT)
gpio.setup(21, gpio.OUT)
# gpio.setup(20, gpio.IN)
gpio.setup(20, gpio.IN, pull_up_down=gpio.PUD_DOWN)
gpio.setup(12, gpio.IN, pull_up_down=gpio.PUD_UP)

gpio.output(16, gpio.LOW)
gpio.output(21, gpio.HIGH)

sleep(1)

gpio.add_event_detect(12, gpio.FALLING)
gpio.add_event_callback(12, restart_klipper)
gpio.add_event_detect(20, gpio.RISING)
gpio.add_event_callback(20, shutdown_printer)

while True:
    sleep(60)
# gpio.wait_for_edge(20, gpio.RISING)
# run_gcode("_SHUTDOWN")
# sleep(120)
# os.system("shutdown -h now")
# sleep(120)
