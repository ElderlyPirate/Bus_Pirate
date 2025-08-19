from debian:stable

arg DEBIAN_FRONTEND=noninteractive
arg COMPILER_URL=https://ww1.microchip.com/downloads/aemDocuments/documents/DEV/ProductDocuments/SoftwareTools/xc16-v2.00-full-install-linux64-installer.run
arg INSTALL_PATH=/opt/microchip/xc16

RUN apt-get update \
	&& apt-get install -y --no-install-recommends \
	bear \
	curl \
	make \
	unzip \
	&& apt-get autoremove \
	&& apt-get autoclean \
	&& rm -rf /var/lib/apt/lists/*

run curl -k ${COMPILER_URL} --output /tmp/xc16-installer.run \
    && chmod 775 /tmp/xc16-installer.run \
    && echo -e "\n\n\n\n\n\n\n\n\n\n\n\n\n\ny\n1\n${INSTALL_PATH}\ny\n\nn\nn\nn\n\n" | /tmp/xc16-installer.run \
    && rm /tmp/xc16-installer.run

workdir /work
env PATH=$PATH:${INSTALL_PATH}/bin
entrypoint ["bear", "--", "make"]
