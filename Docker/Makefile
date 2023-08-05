

all: .phony
	docker build -t cold-arc-devenv .

run:
	docker run -d --rm --name cold-arc-devcontainer \
	 --mount "type=bind,source=$(shell realpath $(shell pwd)/..),target=/home/developer/cold-arc" \
	 --env="DISPLAY" \
	 --volume="$(HOME)/.Xauthority:/home/developer/.Xauthority:rw" \
	 --volume="/tmp/.X11-unix:/tmp/.X11-unix" \
	 cold-arc-devenv

.phony: