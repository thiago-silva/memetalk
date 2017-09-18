FROM debian

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get -qy update && apt-get -qy --fix-missing install \
    build-essential git pkg-config xvfb libgc-dev libatomic-ops-dev \
    libqt4-dev libqtwebkit-dev libqscintilla2-dev \
    libboost-system-dev libboost-filesystem-dev libboost-iostreams-dev \
    python-pip gdb

RUN mkdir /build

RUN /bin/bash -c "pip install \
    ejson \
    clime \
    git+git://github.com/thiago-silva/pymeta.git"

WORKDIR /build

RUN echo '{"repositories":                             \
  {"central": "http://libraries.memetalk.org/"},       \
 "override_to_local": {                                \
   "central:memescript": "/build/central/memescript",  \
   "central:stdlib": "/build/central/stdlib"           \
}}' >> /root/.meme.config

# Runs tests under xvfb to allow Qt to connect to a display
CMD xvfb-run --server-args="-screen 0 1024x768x24" make test
