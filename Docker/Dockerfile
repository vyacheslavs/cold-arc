FROM ubuntu:latest

RUN adduser --debug --system --group --home /var/lib/colord colord --quiet

RUN apt-get update && apt-get install -y build-essential libgtkmm-3.0-dev
RUN apt-get install -y mc

RUN groupadd -g 1000 developer
RUN useradd -rm -d /home/developer -s /bin/bash -g developer -G sudo -u 1000 developer

USER developer
WORKDIR /home/developer

ENTRYPOINT ["/bin/sh","-c","sleep infinity"]



