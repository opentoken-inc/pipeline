THIS_BIN:=sender
CPP=$(wildcard $(ROOT)*.cc) $(wildcard $(ROOT)gason/*.cc)
include ./common.mk

all: sender receiver wsscat wssreplay

receiver:
	make -C ./receiver

wsscat:
	make -C ./wsscat

wssreplay:
	make -C ./wssreplay
