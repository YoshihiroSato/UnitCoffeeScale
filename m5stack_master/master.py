import os, sys, io
import M5
from M5 import *
from hardware import UART



uart0 = None


def setup():
  global uart0

  M5.begin()
  uart0 = UART(0, baudrate=9600, bits=8, parity=None, stop=1, tx=2, rx=1)


def loop():
  global uart0
  M5.update()
  print(uart0.read())


if __name__ == '__main__':
  try:
    setup()
    while True:
      loop()
  except (Exception, KeyboardInterrupt) as e:
    try:
      from utility import print_error_msg
      print_error_msg(e)
    except ImportError:
      print("please update to latest firmware")
