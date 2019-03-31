#!/bin/bash

ROUTER_IP_MAIN='192.168.0.1'
ROUTER_IP_BACKUP='192.168.0.100'

function generateDnsmasqConf() {
  cp dnsmasq.conf.example dnsmasq.conf.main
  cp dnsmasq.conf.example dnsmasq.conf.backup
}

sudo apt-get -y install dialog

sudo apt install dnsmasq
# dhcp-option=3,1.2.3.4

