# About
libpwrm is an embeddable library, CLI app and daemon for realtime, decentralized measuring and reporting on CPU and GPU hardware electricity consumption by blockchain nodes without requiring specialized hardware power meters. libpwrm provides software-based continuous monitoring of different hardware power consumption interfaces and automatic reporting of the measurement data to the [Ceramic](https://ceramic.network/) data network.

libpwrm can [read](https://github.com/allisterb/libpwrm/blob/master/src/measurement/measurement.cpp) power consumption data from [different hardware power meters](https://github.com/allisterb/libpwrm/tree/master/src/measurement) including laptop power supply meters. It can also [read](https://github.com/allisterb/libpwrm/tree/master/src/cpu) CPU power consumption data from Intel's Running Average Power Limit – [RAPL](https://01.org/blogs/2014/running-average-power-limit-%E2%80%93-rapl) interface which uses power metering builtin to Intel CPUs, and NVIDIA GPU power consumption data using the builtin GPU power meters and the [NVML](https://developer.nvidia.com/nvidia-management-library-nvml) interface.

libpwrm runs continuously and publishes the power consumption data it collects for each system it runs on as data streams on Ceramic. Each data document is signed using a [decentralized identifier](https://developers.ceramic.network/learn/glossary/#dids) (DID) which authenticates the entity creating it and contains information that the blockchain node operator provides like a unique miner ID and geographic information together with up-to-date information on the current electricity consumption of their hardware. This allows individual blockchain node operators like Filecoin storage providers to automate reporting their electrical consumption data estimates in realtime without requiring installation of external power metering hardware or complex software or making changes to their existing processes.


The aggregated data collected can also provide a useful complement to existing methods of estimating power consumption of an entire blockchain network at a particular time.


# Getting started

## Requirements
1. C++11 toolchain
2. CMake 3.17 or greater.
3. To monitor NVIDIA graphics card you'll need the [CUDA Toolkit](https://developer.nvidia.com/cuda-downloads?target_os=Linux) installed.

## Installation
1. Clone the git repository `git clone https://github.com/allisterb/libpwrm.git --recurse-submodules`.
2. Run the `build-debug` script from the repo folder.
3. Run `./pwrm --help` from the repo folder to see the available commands.
