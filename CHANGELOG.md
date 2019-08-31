# Pulsemeter changelog

## v1.4.4 - 2019-08-31
* rename project from pulse to pulsemeter
* pulsemeter create directory of rrd file
* no throw on failed upload to pvoutput

## v1.4.3 - 2019-08-31
* use rrdc_flush instead of rrd_flush_if_daemon
* depend on rrdcached.service
* both rrdcached and pulse are run as root (for now)

## v1.4.2 - 2019-08-30
* create obj directory in Makefile

## v1.4.1 - 2019-08-30
* updated Makefile

## v1.4.0 - 2019-08-30
* initial release
