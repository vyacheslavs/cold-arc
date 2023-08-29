FROM ubuntu:latest

RUN adduser --debug --system --group --home /var/lib/colord colord --quiet

RUN apt-get update && apt-get install -y gnupg curl

RUN apt-get update && apt-get install -y build-essential libgtkmm-3.0-dev mc git libsqlite3-dev sqlite3 libxml2-utils libssl-dev
RUN apt-get install -y dbus-x11 valgrind sudo gdb
RUN apt-get install -y flatpak flatpak-builder
RUN apt-get install -y autotools-dev automake libtool

RUN groupadd -g 1000 developer
RUN useradd -r -d /home/developer -s /bin/bash -g developer -G sudo -u 1000 developer
RUN echo "developer:pass" | chpasswd

USER developer
WORKDIR /home/developer

ENTRYPOINT ["/bin/sh","-c","sleep infinity"]



