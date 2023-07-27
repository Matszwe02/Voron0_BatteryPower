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
    run_gcode("OFF")
    sleep(1)
    run_gcode("M112")
    sleep(0.2)
    run_gcode("firmware_restart")


gpio.setmode(gpio.BCM)

gpio.setup(16, gpio.OUT)
gpio.setup(21, gpio.OUT)
gpio.setup(20, gpio.IN)

gpio.output(16, gpio.LOW)
gpio.output(21, gpio.HIGH)

sleep(1)

# if you want to wire the v0 display kill switch to restart firmware instead of halting it:
# gpio.setup(12, gpio.IN, pull_up_down=gpio.PUD_UP)
# gpio.add_event_detect(12, gpio.FALLING)
# gpio.add_event_callback(12, restart_klipper)


gpio.wait_for_edge(20, gpio.RISING)
run_gcode("SHUTDOWN")
sleep(120)
os.system("shutdown -h now")
sleep(120)
