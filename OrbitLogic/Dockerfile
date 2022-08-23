#FROM ubuntu:18.04 as base
FROM harisekhon/ubuntu-java:8 as base

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
RUN mkdir -p $INSTALL && \
    mkdir -p ${INSTALL}/cFS && \
    mkdir -p ${INSTALL}/Database && \
    mkdir -p ${INSTALL}/Demo && \
    mkdir -p ${INSTALL}/Development && \
    mkdir -p ${INSTALL}/Docs && \
    mkdir -p ${INSTALL}/Include && \
    mkdir -p ${INSTALL}/InOut && \
    mkdir -p ${INSTALL}/Kit && \
    mkdir -p ${INSTALL}/License && \
    mkdir -p ${INSTALL}/Model && \
    mkdir -p ${INSTALL}/MonteCarlo && \
    mkdir -p ${INSTALL}/Object && \
    mkdir -p ${INSTALL}/Potato && \
    mkdir -p ${INSTALL}/Rx && \
    mkdir -p ${INSTALL}/Source && \
    mkdir -p ${INSTALL}/Standalone && \
    mkdir -p ${INSTALL}/Tx && \
    mkdir -p ${INSTALL}/Utilities && \
    mkdir -p ${INSTALL}/World

COPY cFS/ ${INSTALL}/cFS/
COPY Database/ ${INSTALL}/Database/
COPY Demo/ ${INSTALL}/Demo/
COPY Development/ ${INSTALL}/Development/
COPY Docs/ ${INSTALL}/Docs/
COPY Include/ ${INSTALL}/Include/
COPY InOut/ ${INSTALL}/InOut/
COPY Kit/ ${INSTALL}/Kit/
COPY License/ ${INSTALL}/License/
COPY Model/ ${INSTALL}/Model/
COPY MonteCarlo/ ${INSTALL}/MonteCarlo/
COPY Object/ ${INSTALL}/Object/
COPY Potato/ ${INSTALL}/Potato/
COPY Rx/ ${INSTALL}/Rx/
COPY Source/ ${INSTALL}/Source/
COPY Standalone/ ${INSTALL}/Standalone/
COPY Tx/ ${INSTALL}/Tx/
COPY Utilities/ ${INSTALL}/Utilities/
COPY World/ ${INSTALL}/World/

COPY README.* ${INSTALL}/
COPY Makefile ${INSTALL}/

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
RUN mkdir -p /docker-entrypoint.d
RUN chmod 777 /docker-entrypoint.d
COPY OrbitLogic/docker-entrypoint.sh /
RUN chmod 755 /docker-entrypoint.sh

# Add nominal startup scripts
RUN mkdir -p /startup
COPY OrbitLogic/startup/ /startup/
RUN chmod 755 /startup/*.sh
RUN chmod 755 /startup/*.pl

# Add gRPC server utilities
RUN mkdir -p /grpc_server
COPY OrbitLogic/grpc_server/ /grpc_server/
RUN chmod -R 755 /grpc_server/bin

# Set up scenario and log locations
RUN mkdir -p /42/scenarios && chmod -R 666 /42/scenarios
RUN mkdir -p /42/logs && chmod -R 777 /42/logs
RUN mkdir -p /scenarios/base

# Default configuration files and application inputs
RUN mkdir -p /config
COPY OrbitLogic/config/ /config/

# Default configuration files and application inputs
RUN mkdir -p /entrypoint-config.d
COPY OrbitLogic/entrypoint-config.d/ /entrypoint-config.d/
RUN chmod -R 644 /entrypoint-config.d

# Create a default/fallback scenario
# Note that the scenarios directory must be relative to the 42 root!
RUN mkdir -p /42/scenarios/default
COPY OrbitLogic/default_scenario/ /42/scenarios/default/

# Scenario Generator
RUN mkdir -p /scenario_generator
COPY OrbitLogic/scenario_generator/ /scenario_generator/
RUN chmod -R 755 /scenario_generator/bin

# Default environment vars
ENV FORTYTWO_SCENARIO=scenarios/default
ENV FORTYTWO_GRPC_CONFIG=/config/server.json
ENV SCENARIO_GENERATOR_DIR=/scenario_generator
ENV SCENARIO_GENERATOR_CONFIG=""
ENV SCENARIO_GENERATOR_COE_SELECTOR=""
ENV SCENARIO_GENERATOR_COE_INPUT=/config/coe_inputs.csv

# Environment setup
ENV LD_LIBRARY_PATH="${INSTALL}/lib:${LD_LIBRARY_PATH}"
ENV LIBGL_ALWAYS_INDIRECT=0
ENV GRAPHICS_FRONT_END=false

WORKDIR /

ENTRYPOINT ["/docker-entrypoint.sh"]
