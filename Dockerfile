FROM alpine:latest AS base

WORKDIR /app

RUN apk add --no-cache \
		qt6-qtbase-dev \
        qt6-qtbase-sqlite \
        qt6-qthttpserver-dev \
        cmake \
		g++ \
		make

RUN apk add --no-cache \
        qt6-qtbase-postgresql \
        libpq libpq-dev

ENV PATH=/usr/lib/qt6/bin:$PATH
EXPOSE 3000
COPY src src
COPY include include
COPY build.sh .
COPY CMakeLists.txt .
RUN ./build.sh
CMD ["/app/./rinha_cpp"]

FROM alpine:latest AS prod
RUN apk add --no-cache \
		qt6-qtbase \
        qt6-qtbase-sqlite \
        qt6-qthttpserver \
		g++ \
        qt6-qtbase-postgresql \
        libpq

COPY --from=base /app/rinha_cpp /usr/bin/rinha_cpp
EXPOSE 3000
CMD ["rinha_cpp"]