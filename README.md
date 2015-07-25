Muni Minutes 2.0
================

## Overview

This is my second attempt at a train-arrival indicator. It uses a [Particle
Core](https://store.particle.io/?product=spark-core) and the associated web
services to display arrival times as a gauge using a servo.

The device makes an http request to the server, which in turn fetches train
arrival times from the [nextbus
api](http://nextbus.com/xmlFeedDocs/NextBusXMLFeed.pdf), using some
configuration from the server.

For the TL;DR of the NextBus XML feed: https://gist.github.com/grantland/7cf4097dd9cdf0dfed14

### Device

The device makes an http request periodically to `http://${SERVER}/times/${DEVICE_NAME}`
The expected result is an http body with the number of minutes to the next train.
The time is then represented by the device in some way (in the simple case, a servo).

### Server Configuration

Server proxies requests for the device to the nextbus xml feed, and emits the
number of minutes until the next train.  `config.yml` has some configuration to
determine which bus/train stop data to fetch:

```
devicename:
    :a: 'sf-muni'
    :r: 'N'
    :s: '8888'
```

devicename is the key, and the hash below (with `:` prefix on key names, so it
is symbolized properly) are arguments added to the nextbus xml feed request.

## To Do

### Device
- [x] find bug with reading http body
- [ ] handle non-200 error codes gracefully.
- [x] change servo delay based on angle delta
- [ ] unit tests
- [ ] Build muni train enclosure
- [ ] add LEDs for lighting up when time is running short.

### Server
- [x] Allow multiple devices
- [x] Configuration done by yaml
