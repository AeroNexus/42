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
#-----------------------------------------------------------------------------

.PHONY: help prune dist push

# Note tha the help function is driven by '##' comments in line with each rule
help: ## See https://marmelab.com/blog/2016/02/29/auto-documented-makefile.html
	@echo "Makefile for $(APP_NAME) build process"
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z_-]+:.*?## / {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}' $(MAKEFILE_LIST)

.DEFAULT_GOAL := help

# Main rules

all: docker-image prune

docker-image: ## Build the container images
	$(eval IMG_TAG := $(URL)/$(APP_NAME))
	docker build -t $(IMG_TAG):$(VERSION) -f Dockerfile $(ROOT)
	docker tag $(IMG_TAG):$(VERSION) $(IMG_TAG):latest

prune: ## Prune any dangling docker images
	@echo "Removing any dangling images..."
	@docker rmi -f $(docker images --filter "dangling=true" -q --no-trunc) > /dev/null 2>&1 | true

clean: clean-images prune ## Remove temporary build files

clean-images: ## Force remove all edge image tags
	@echo "Removing all image tags..."
	@for tag in $(ALL_IMGS); do docker rmi -f $$tag >/dev/null 2>&1; done | true

# Helpers

show-vars: ## Report all variable definitions
	@echo APP_NAME=$(APP_NAME)
	@echo URL=$(URL)
	@echo VERSION=$(VERSION)
	@echo ROOT=$(ROOT)
	@echo ALL_IMGS=
	@for img in $(ALL_IMGS); do echo $$img; done