# Import configuration file
# override with e.g. `make cnf="my.env" build`
cnf ?= .env
include $(cnf)
export $(shell sed 's/=.*//' $(cnf))
#-----------------------------------------------------------------------------
APP_NAME := 42
URL      := $(REGISTRY_URL)
VERSION  := $$(git log -1 --pretty=%h)
ROOT     := $$(cd ../ && pwd)
ALL_IMGS := $$(docker image list | grep $(URL) | awk '{printf "%s:%s\n", $$1, $$2}')
PLATFORMS := linux/arm64,linux/amd64
#-----------------------------------------------------------------------------

.PHONY: help prune dist push

# Note tha the help function is driven by '##' comments in line with each rule
help: ## See https://marmelab.com/blog/2016/02/29/auto-documented-makefile.html
	@echo "Makefile for $(APP_NAME) build process"
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z_-]+:.*?## / {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}' $(MAKEFILE_LIST)

.DEFAULT_GOAL := help

setup-buildx:  ## Set up for docker buildx environment
	@echo Setting up buildx docker service...
	sed 's/your\.local:port/$(URL)/g' build/example.buildkitd.toml > build/buildkitd.toml
	docker buildx rm mybuilder 2>/dev/null || true
	docker buildx create --name mybuilder --config build/buildkitd.toml --use
	docker buildx inspect --bootstrap

# Main rules

all: docker-image prune

docker-image: ## Build the container images
	$(eval IMG_TAG := $(URL)/$(APP_NAME))
	docker buildx build -o type=docker --platform=linux/amd64 --progress=plain \
		--cache-from=$(URL)/$(APP_NAME):latest \
		-t $(IMG_TAG):$(VERSION) \
		-t $(IMG_TAG):latest \
		-f Dockerfile $(ROOT)

docker-multiplatform: ## Build the multiplatform container images
	$(eval IMG_TAG := $(URL)/$(APP_NAME))
	docker buildx build --push --platform=$(PLATFORMS) --progress=plain \
		--cache-from=$(URL)/$(APP_NAME):latest \
		-t $(IMG_TAG):$(VERSION) \
		-t $(IMG_TAG):latest \
		-f Dockerfile $(ROOT)

# Program-specific rules

luna-example: ## Build program-specific container image
	$(eval IMG_TAG := $(URL)/$(APP_NAME):luna_example)
	docker build -t $(IMG_TAG) \
		-t $(IMG_TAG) \
		-f Dockerfile-luna-example .

# Helpers

prune: ## Prune any dangling docker images
	@echo "Removing any dangling images..."
	@docker rmi -f $(docker images --filter "dangling=true" -q --no-trunc) > /dev/null 2>&1 | true

clean: clean-images prune ## Remove temporary build files

clean-images: ## Force remove all edge image tags
	@echo "Removing all image tags..."
	@for tag in $(ALL_IMGS); do docker rmi -f $$tag >/dev/null 2>&1; done | true

show-vars: ## Report all variable definitions
	@echo APP_NAME=$(APP_NAME)
	@echo URL=$(URL)
	@echo VERSION=$(VERSION)
	@echo ROOT=$(ROOT)
	@echo ALL_IMGS=
	@for img in $(ALL_IMGS); do echo $$img; done
