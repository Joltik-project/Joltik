# Joltik: Enabling Energy-Efficient “Future-Proof” Analytics on Low-Power Wide-Area Networks

Authors: Mingran Yang, Junbo Zhang, Akshay Gadre, Zaoxing Liu, Swarun Kumar, and Vyas Sekar

## Introduction

This repository contains code and Joltik dataset for MobiCom'20 paper Joltik: Enabling Energy-Efficient “Future-Proof” Analytics on Low-Power Wide-Area Networks. Link to the paper: https://dl.acm.org/doi/10.1145/3372224.3419204

For any questions to this repository or to the paper, please contact Mingran Yang (mingrany@mit.edu).

> ACM Reference format:
Mingran Yang, Junbo Zhang, Akshay Gadre, Zaoxing Liu, Swarun Kumar, and Vyas Sekar. 2020. Joltik: Enabling Energy-Efficient “Future-Proof” Analytics on Low-Power Wide-Area Networks. In The 26th Annual International Conference on Mobile Computing and Networking (MobiCom ’20), September 21–25, 2020, London, United Kingdom. ACM, New York, NY, USA, 14 pages. https://doi.org/10.1145/3372224.3419204

## Running Joltik optimized universal sketching algorithms on PC
optimized_universal_sketching folder contains the code for algorithms used in Joltik. 

### Add your sensor datasets
Please do so by editing optimized_universal_sketching/helper/input.h. Please notice that before sending sensed value to Joltik optimized universal sketching algorithms, please sclae the value to integer.

### Change sketch related parameters
Please do so by editing optimized_universal_sketching/helper/sketch_config.h

### Instructions for simulation on PC
If you want to simulate Joltik on PC, please use the Makefile located in "optimized_universal_sketching". After make, you can go to "optimized_universal_sketching/bin", and the executable file "testSketch" is for universal sketching online algorithm, and "estiSketch" is for universal sketching offline algorithm. The commands are as follow:
```
cd optimized_universal_sketching
make
cd bin
./testSketch  
./estiSketch
```

## Running Joltik optimized universal sketching algorithms on sensors
### Change sketch related parameters
Please do so by editing optimized_universal_sketching/helper/sketch_config.h

### Sensor node

If you want to run universal sketching online algorithm on sensors, please include the following files as library in the sensor code on sensor node:
```
optimized_universal_sketching/univmon.c
optimized_universal_sketching/univmon.h
optimized_universal_sketching/helper/*
```
And you can change the main function in "optimized_universal_sketching/univmon.c" as your sensor's main function.

### Base station
If you want to run universal sketching offline algorithm on base station or PC, please include the following files as library in the base station:
```
optimized_universal_sketching/univmon_offline.c
optimized_universal_sketching/univmon.h
optimized_universal_sketching/helper/*
```
And you can change the main function in "optimized_universal_sketching/univmon_offline.c" as your base station's main function.

## Joltik dataset
Joltik dataset is the pressure dataset we collect using Joltik sensor node on campus. Our joltik sensor node includes sensor board (X-NUCLEO-IKS01A2), MCU (NUCLEO-L476RG) and RF frontend (SX1276 LoRa Transceiver). Please refer to Figure 7 in our paper for hardware components of Joltik.

In "dataset/joltik_dataset/" folder, we include the datasets we collected using 10 sensor nodes in our proof-of-concept testbed on campus, and sensor locations is shown in Figure 8 of our paper. The dataset collected by each sensor is named as "sensor_node_x". All sensor nodes are operating at 1Hz sampling frequency for 10 days.
