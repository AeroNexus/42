# Orbit Logic 42 Configuration

This folder contains extensions of the 42 simulator provided by Orbit Logic Inc.

## Quick Start

Copy the provided `.env.example` file to a `.env` file in the top level of the repository. Ensure that the URL for your private registry is correct.

Use the `Makefile` to build the generic Docker image.
```bash
make docker-image
```

Simply run the docker image in a container to use the default scenario. In most cases, you will want to pass a variety of environment settings; see the [Example Compose](#example-compose-content) section below.
```bash
docker run --rm cogit-art.orbitlogic.com:5050/aps/store/42:latest
```

## Automatic Scenario Generation

### How this Works

The Orbit Logic build of the 42 simulation framework includes a **Scenario Generator** tool that can generate the required 42 input files (flat text files with custom format) using a single YAML configuration file. For an example, see `scenario_generator/config/ExampleScenario.yaml`.

The Scenario Generator is invoked automatically if the SCENARIO_GENERATOR_CONFIG variable is defined and the specified path/file is valid.

At container startup, the `startup/10_generate_scenario.sh` script checks this path, and invokes the Scenario Generator before 42 is started. The example configuration YAML uses `/42/scenarios/default` as the output path, matching the default FORTYTWO_SCENARIO definition. It is recommended that you keep with this same (default) generation path to avoid mismatches.

With a populated `scenarios/default` directory, the `docker-entrypoint.sh` script invokes the 42 simulation in the following manner:

```bash
cd /42 && ./42 $FORTYTWO_SCENARIO | tee -a ${LOGFILE}
```

See the [Docker Compose Examples](#example-compose-content) section below.

### Startup Scripts

Scripts are run via Docker entrypoint in the following order:

1. Alphanumerical order for any `.sh` file in the `/startup` directory
2. Alphanumerical order for any `.sh` file in the `/docker-entrypoint.d` directory

### Configuration Files

Any files placed in the `entrypoint-config.d` directory will be auto-installed to the `/config` directory in the running container instance.

Furthermore, basic suppport for variable substitution is supported. This means you can define a configuration file under `/config` as follows:

```javascript
// /config/my-config.template.json**
{
  "myValue" : "$MY_VARIABLE"
}
```
and the value for MY_VARIABLE at will be injected into the configuration file at startup. Note that the default gRPC server configuration file `/config/server.json` is generated from a template using this same approach from a `server.template.json`.

See the scripts under `startup` for details.

### Other Key Files
- **.env** - Sets environment variables used by the Makefile and docker-compose
- **Dockerfile** - Used to build the Orbit Logic 42 container image
- **Makefile** - Used for convenience to support build process
- **docker-entrypoint.sh** - Container entrypoint script

## Graphics Settings

When using the graphical UI in a container or VM, pass the host display for x-window support. This nominally looks like `172.x.y.z:0` in WSL.

```yaml
    environment:
      - DISPLAY=${DISPLAY}
      - GRAPHICS_FRONT_END=true
```
To disable graphics and run headless use the following.

```yaml
    environment:
      - GRAPHICS_FRONT_END=false
```

## Environment Variable Settings

`FORTYTWO_SCENARIO`

Use this to set the simulation scenario root, *relative to the 42 root directory*. Default=scenario/default.

`FORTYTWO_GRPC_CONFIG`

Use this to specify the path to the GRPC server configuration file. This file is required for sending satellite state data externally over GRPC/TCP ports.

`FORTYTWO_AUTO_RESTART`

Set to value = "true" to automatically restart the simulation scenario upon expiration of the scenario time span. Default=false.

`SCENARIO_GENERATOR_CONFIG`

Use this to specify the Scenario Generator configuration YAML. This file is used as input to the generator to create artifacts defining each satellite asset, initial attitude, etc.

`SCENARIO_GENERATOR_COE_INPUT`

Use this to specify the input orbit definitions used by the Scenario Generator. This file is a CSV in the following format:

```csv
"SatName","Perigee Altitude (km)","Apogee Altitude (km)","Inclination (deg)","RAAN (deg)","Arg of Perigee (deg)","True Anomaly (deg)"
DemoSat-11,7078.137000,7078.137000,74.959,201.750,0.000,70.389
DemoSat-12,7078.137000,7078.137000,74.959,201.750,0.000,130.389
```

`SCENARIO_GENERATOR_COE_SELECTOR`

Use this to downselect a specific entry from the COE_INPUT file. This is useful for deployments where you only need one satellite in the simulation; for instance, satellite pods deployed to a k3s environment may each have their own dedicated simulator.

`GRAPHICS_FRONT_END`

Set to `false` to disable the simulator graphics engine. No UI windows will be displayed, and the simulation will run "headless." This is useful for conserving system resources, or for running the simulation on a machine that does not have adequate GPU capability.

#### Important Notes

Note that when the Scenario Generator is used, a file is created at runtime called `/config/coe.csv`. This generated file MUST be referenced in the scenario generation YAML, like as follows:

```yaml
assets:
    # Assets may be provided as an Asset File or list of Spacecrafts. A List provided spacecraft can override
    # a file asset if named identically
    file               :
        path           : '/config/coe.csv'
```
Referencing the input COE file in this path will prevent the asset downselection from working.

## Logging

Logs for each individual process are stored under `/42/logs` in the running container. You may choose to mount this directory to the local file system, or pull logs from this location during test runtime.

## Appendix

### Program-Specific Builds

It may be convenient to create a program-specific Docker image where all scenario files are prepackaged into the image, reducing the amount of environment variable settings required to start the simulation. This can also be used to lock down a specific simulation if the target demo/test user should not be overriding the scenario settings.

For a program-specific deployment, see the `Dockerfile-luna-example`. This image extends extends the base image to include program-specific content stored under the `scenarios` directory.

```bash
make luna-example
```

### Example Compose Content

Note: the following examples omit any variables where the default is sufficient.

#### Run from Static Scenario Directory

```yaml
services:
  fortytwo:
    container_name: fortytwo
    image: cogit-art.orbitlogic.com:5050/aps/store/42
    volumes:
      # Any configs placed in the entrypoint-config.d location are auto-installed to /config
      - /path/to/my/configs:/entrypoint-config.d
      # Mount in pre-created scenario files
      - /path/to/my/scenario:/42/scenarios/myscenario
    environment:
      - FORTYTWO_SCENARIO=scenarios/myscenario
      - FORTYTWO_GRPC_CONFIG=/config/MyServerConfig.json
      # No scenario generation; files under scenarios/myscenario will be used verbatim
```

#### Generate and Run Constellation

The following is an example `docker-compose` YAML service definition that leverages the scenario generator tool.

```yaml
services:
  fortytwo:
    container_name: fortytwo
    image: cogit-art.orbitlogic.com:5050/aps/store/42
    volumes:
      # Any configs placed in the entrypoint-config.d location are auto-installed to /config
      - /path/to/my/configs:/entrypoint-config.d
    environment:
      - FORTYTWO_GRPC_CONFIG=/config/MyServerConfig.json
      - SCENARIO_GENERATOR_CONFIG=/config/MyScenario.yaml
      - SCENARIO_GENERATOR_COE_INPUT=/config/my_coe_file.csv
      # Content of /config/coe.csv will be entire constellation from input file
```

#### Generate and Select Single Satellite

```yaml
services:
  fortytwo:
    container_name: fortytwo
    image: cogit-art.orbitlogic.com:5050/aps/store/42
    volumes:
      # Any configs placed in the entrypoint-config.d location are auto-installed to /config
      - /path/to/my/configs:/entrypoint-config.d
    environment:
      # Use default FORTYTWO_GRPC_CONFIG to leverage variable replacement
      - SCENARIO_GENERATOR_CONFIG=/config/MyScenario.yaml
      - SCENARIO_GENERATOR_COE_INPUT=/config/my_coe_file.csv
      # Reduce the COE file to use only the row matching this name
      - SCENARIO_GENERATOR_COE_SELECTOR=DemoSat-11
      # Content of /config/coe.csv will be reduced to DemoSat-11 row
```