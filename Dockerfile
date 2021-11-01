FROM ubuntu:18.04 as base

# Build in a temporary build container
FROM base as build

ARG DEBIAN_FRONTEND=noninteractive

RUN apt update && apt-get install -y \
    vim-gtk \
    gcc \
    g++ \
    freeglut3-dev \
    libbsd-dev \
    make \
&& rm -rf /var/lib/apt/lists/*

# This repo is flat, so we need to copy specific folders
ARG INSTALL="/42"
RUN mkdir -p $INSTALL

COPY cFS "${INSTALL}/cFS"
COPY Database "${INSTALL}/Database"
COPY Demo "${INSTALL}/Demo"
COPY Development "${INSTALL}/Development"
COPY Include "${INSTALL}/Include"
COPY InOut "${INSTALL}/InOut"
COPY Kit "${INSTALL}/Kit"
COPY License "${INSTALL}/License"
COPY Model "${INSTALL}/Model"
COPY Object "${INSTALL}/Object"
COPY Potato "${INSTALL}/Potato"
COPY Rx "${INSTALL}/Rx"
COPY Source "${INSTALL}/Source"
COPY Standalone "${INSTALL}/Standalone"
COPY Tx "${INSTALL}/Tx"
COPY Utilities "${INSTALL}/Utilities"
COPY World "${INSTALL}/World"

COPY Makefile $INSTALL
WORKDIR $INSTALL
RUN make

#=====================================================
# Copy built 42 artifacts to base container
FROM base

# Install select tools we want in the final image
RUN apt update && apt-get install -y \
    net-tools \
    tcpdump \
&& rm -rf /var/lib/apt/lists/*

ARG INSTALL="/42"
RUN mkdir -p $INSTALL
COPY --from=build /42 $INSTALL

ARG LIB_INSTALL="/usr/lib/x86_64-linux-gnu"
RUN mkdir -p ${LIB_INSTALL}

# Specific dependencies (can limit image size, but YMMV) 
#COPY --from=build [ \
#    "/usr/lib/x86_64-linux-gnu/libGL.so.1", \
#    "/usr/lib/x86_64-linux-gnu/libGLdispatch.so.0", \
#    "/usr/lib/x86_64-linux-gnu/libGLU.so.1", \
#    "/usr/lib/x86_64-linux-gnu/libGLX.so.0", \
#    "/usr/lib/x86_64-linux-gnu/libglut.so.3", \
#    "/usr/lib/x86_64-linux-gnu/libXi.so.6", \
#    "/usr/lib/x86_64-linux-gnu/libX11.so.6", \
#    "/usr/lib/x86_64-linux-gnu/libXxf86vm.so.1", \
#    "/usr/lib/x86_64-linux-gnu/libxcb.so.1", \
#    "/usr/lib/x86_64-linux-gnu/libXau.so.6", \
#    "/usr/lib/x86_64-linux-gnu/libXdmcp.so.6", \
#    "/usr/lib/x86_64-linux-gnu/libXext.so.6", \
#    "${LIB_INSTALL}/" \
#]

# Copy in dependencies under /usr/lib from build image
COPY --from=build [ \
    "/usr/lib/x86_64-linux-gnu", \
    "${LIB_INSTALL}" \
]

# Copy in dependencies under /lib from build image
ARG LIB_INSTALL="/lib/x86_64-linux-gnu"
RUN mkdir -p ${LIB_INSTALL}

COPY --from=build [ \
    "/lib/x86_64-linux-gnu", \
    "${LIB_INSTALL}" \
]

# Docker entrypoint script and hooks
COPY docker-entrypoint.sh /
RUN mkdir -p /docker-entrypoint.d

COPY docker-entrypoint.d /
RUN chmod 755 /docker-entrypoint.sh

# Default scenarios location
RUN mkdir -p /42/scenarios
RUN chmod -R 666 /42/scenarios
ENV FORTYTWO_SCENARIO=scenarios/default
# Note that the scenarios directory must be relative to the 42 root!

# Environment setup
ENV LD_LIBRARY_PATH="${INSTALL}/lib:${LD_LIBRARY_PATH}"
ENV LIBGL_ALWAYS_INDIRECT=0

WORKDIR /

ENTRYPOINT ["/docker-entrypoint.sh"]
