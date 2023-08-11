

all: .phony
	docker build -t cold-arc-devenv .

run:
	docker run -d --rm --name cold-arc-devcontainer \
	 --mount "type=bind,source=/home/slavs/workspace/cold-arc-gtk,target=/home/developer" \
	 --env="DISPLAY" \
	 --ipc=host \
	 --hostname cold-arc \
	 --privileged \
	 --volume="$(HOME)/.Xauthority:/home/developer/.Xauthority:rw" \
	 --volume="/tmp/.X11-unix:/tmp/.X11-unix" \
	 cold-arc-devenv

con: .phony
	docker exec -ti cold-arc-devcontainer /bin/bash

loop: /dev/loop0 loop.img
	losetup /dev/loop0 loop.img

/dev/loop0:
	mknod /dev/loop0 b 7 0

loop.img:
	dd if=/dev/zero of=./loop.img bs=1M count=5120

.phony:
