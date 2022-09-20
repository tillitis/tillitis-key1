build:
	docker build -t key1 .

run:
	docker run --mount type=bind,source="`pwd`/../",target=/build -w /build -it key1 /usr/bin/bash
