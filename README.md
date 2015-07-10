Muni Minutes 2.0
================

## Overview

This is my second attempt at a train-arrival indicator. It uses a [Particle
Core](https://store.particle.io/?product=spark-core) and the associated web
services to display arrival times as a gauge using a servo.

## To Do

### Device
- [ ] find bug with reading http body
- [ ] change servo delay based on angle delta
- [ ] unit tests
- [ ] Build muni train enclosure
- [ ] add LEDs for lighting up when time is running short.

### Server
- [x] Allow multiple devices
- [x] Configuration done by yaml
