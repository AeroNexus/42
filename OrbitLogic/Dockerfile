# Use the java base container to support scenario generator
#FROM harisekhon/ubuntu-java:8 as base
FROM eclipse-temurin:8 as base

# Build in a temporary build container
FROM base as build

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    vim-gtk3 \
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
RUN apt-get update && apt-get install -y \
    net-tools \
    tcpdump \
    netcat-traditional \
&& rm -rf /var/lib/apt/lists/*

ARG INSTALL="/42"
RUN mkdir -p $INSTALL
COPY --from=build /42 $INSTALL

# FIXME: this is a bit overkill; 42 requires the dependencies under </usr>/lib/{arch}-linux-gnu
# The following copies everythign to be safe, but this could be reduced for image size.
COPY --from=build /lib/ /lib/
COPY --from=build /usr/lib/ /usr/lib/

#ARG LIB_INSTALL="/usr/lib/x86_64-linux-gnu"
#RUN mkdir -p ${LIB_INSTALL}
# Copy in dependencies under /usr/lib from build image
#COPY --from=build [ \
#    "/usr/lib/x86_64-linux-gnu", \
#    "${LIB_INSTALL}" \
#]
# Copy in dependencies under /lib from build image
#ARG LIB_INSTALL="/lib/x86_64-linux-gnu"
#RUN mkdir -p ${LIB_INSTALL}
#COPY --from=build [ \
#    "/lib/x86_64-linux-gnu", \
#    "${LIB_INSTALL}" \
#]

#=====================================================
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

# Set up scenario and log locations
RUN mkdir -p /42/scenarios && chmod -R 666 /42/scenarios
RUN mkdir -p /42/logs && chmod -R 777 /42/logs

# Install base scenario artifacts for reference
RUN mkdir -p /42/scenarios/base
COPY OrbitLogic/scenarios/base/ /42/scenarios/base/

#=====================================================
# Add gRPC server utilities

RUN mkdir -p /grpc_server
COPY OrbitLogic/grpc_server/ /grpc_server/
RUN chmod -R 755 /grpc_server/**/bin

#=====================================================
# Default configuration files and application inputs

# Create placeholder config location. Files placed in /entrypoint-config.d
# are automatically installed here, so the directory needs to exist.
RUN mkdir -p /config
RUN chmod -R 755 /config

# Install default grpc_sever config files
COPY OrbitLogic/grpc_server/config/*.json /config

# Default configuration files and application inputs
RUN mkdir -p /entrypoint-config.d
COPY OrbitLogic/entrypoint-config.d/ /entrypoint-config.d/
RUN chmod -R 644 /entrypoint-config.d

#=====================================================
# Create a default/fallback scenario

RUN mkdir -p /42/scenarios/default
COPY OrbitLogic/scenarios/luna_example/ /42/scenarios/default/

#=====================================================
# Scenario Generator

RUN mkdir -p /scenario_generator
COPY OrbitLogic/scenario_generator/ /scenario_generator/
RUN chmod -R 755 /scenario_generator/bin

#=====================================================
# Environment setup

# Default scenario. Note that this is intentionally relative to the 42 root!
ENV FORTYTWO_SCENARIO=scenarios/default

# Default to single-run (do not recycle scenario on completion)
ENV FORTYTWO_AUTO_RESTART=false

# Provide a default gRPC server configuration path
ENV FORTYTWO_GRPC_CONFIG=/config/server.json

# Default scenario generator inputs. These are usually overridden
ENV SCENARIO_GENERATOR_DIR=/scenario_generator
ENV SCENARIO_GENERATOR_COE_INPUT=/scenario_generator/config/coe_inputs.csv

# Default: no scenario generation (blank string)
# When the user provides a file path here, generation is enabled
ENV SCENARIO_GENERATOR_CONFIG=""

# The following is used to downselect assets from the coe_inputs. By default
# it is blank (use all assets in the coe_inputs file)
ENV SCENARIO_GENERATOR_COE_SELECTOR=""

ENV LD_LIBRARY_PATH="${INSTALL}/lib:${LD_LIBRARY_PATH}"
ENV LIBGL_ALWAYS_INDIRECT=0
ENV GRAPHICS_FRONT_END=false

WORKDIR /

ENTRYPOINT ["/docker-entrypoint.sh"]
