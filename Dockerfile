FROM debian:jessie

ADD . /sources
WORKDIR /sources

CMD ["sleep", "infinity"]
