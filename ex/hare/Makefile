THIS_BIN:=sender
CPP=$(wildcard $(ROOT)*.cc) $(wildcard $(ROOT)gason/*.cc)
include ./common.mk

all: sender receiver wsscat

receiver:
	make -C ./receiver

wsscat:
	make -C ./wsscat
