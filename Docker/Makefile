

all: .phony
	docker build -t cold-arc-devenv .

run:
	docker run -d --rm --name cold-arc-devcontainer \
	 --mount "type=bind,source=/home/slavs/workspace/cold-arc-gtk,target=/home/developer" \
	 --env="DISPLAY" \
	 --ipc=host \
	 --hostname cold-arc \
	 --volume="$(HOME)/.Xauthority:/home/developer/.Xauthority:rw" \
	 --volume="/tmp/.X11-unix:/tmp/.X11-unix" \
	 cold-arc-devenv

con: .phony
	docker exec -ti cold-arc-devcontainer /bin/bash

.phony:
