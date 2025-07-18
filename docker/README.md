This folder contains Dockerfile for creation of the toolchain container, which then could be used to build the firmware.

### Create container

In order to create the container, issue the command in `docker` folder
```bash
docker build --tag buspirate:latest -f build.dockerfile .
```

If running behind proxy, you'll need to supply additional arguments
```bash
docker build --tag buspirate:latest --network=host --build-arg=http_proxy=http://127.0.0.1:3128/ --build-arg=https_proxy=http://127.0.0.1:3128/ -f build.dockerfile .
```

This command will download XC16 compiler and install it to the image named `buspirate` with tag `latest`.

If you wish to use different compiler version, modify `COMPILER_URL` argument in `build.dockerfile`.

### Build firmware

Currently only BPv4 firmware was built and tested using Dockerized toolchain. BPv3 might need additional tweaks.

#### BPv4 bootloader

To build BPv4 bootloader proceed to its sources folder and issue the command
```bash
docker run --rm -it -v $PWD:/work buspirate:latest
```
This will create `bpv4-bootloader.hex` firmware image.

To clean the project use
```bash
docker run --rm -it -v $PWD:/work buspirate:latest clean
```
or
```bash
docker run --rm -it -v $PWD:/work buspirate:latest distclean
```

#### BPv4 firmware

To build BPv4 firmware proceed to its sources folder and issue the command
```bash
docker run --rm -it -v $PWD:/work buspirate:latest BP_VERSION=V4
```
This will create `bpv4-bootloader.hex` firmware image.

To clean the project use
```bash
docker run --rm -it -v $PWD:/work buspirate:latest BP_VERSION=V4 clean
```
or
```bash
docker run --rm -it -v $PWD:/work buspirate:latest BP_VERSION=V4 distclean
```
