FROM debian:jessie

ADD maciej_buszka /sources
WORKDIR /sources

CMD ["sleep", "infinity"]
