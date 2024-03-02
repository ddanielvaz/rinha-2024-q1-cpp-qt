SHELL = /bin/bash
.ONESHELL:
.DEFAULT_GOAL: help

help: ## Prints available commands
	@awk 'BEGIN {FS = ":.*##"; printf "Usage: make \033[36m<target>\033[0m\n"} /^[.a-zA-Z_-]+:.*?##/ { printf "  \033[36m%-25s\033[0m %s\n", $$1, $$2 } /^##@/ { printf "\n\033[1m%s\033[0m\n", substr($$0, 5) } ' $(MAKEFILE_LIST)

qt.built: ## Qt Build usin cmake
	@docker compose run --rm qt ./build.sh

dev.up: ## Start the rinha in dev
	@docker compose up -d nginx

dev.down: ## Stop the rinha in dev
	@docker compose down

prod.up: ## Start the rinha in prod
	@docker compose -f docker-compose-prod.yml up -d nginx

prod.down: ## Stop the rinha in prod
	@docker compose -f docker-compose-prod.yml down

docker.stats: ## Show docker stats
	@docker stats --format "table {{.Name}}\t{{.CPUPerc}}\t{{.MemUsage}}\t{{.MemPerc}}"

health.check: ## Check the stack is healthy
	@curl -v http://localhost:9999/clientes/2/extrato

stress.it: ## Run local stress tests
	@sh ./executar-teste-local.sh

docker.build: ## Build the docker image
	@docker build -t danielvazzz/rinha-2024-q1-cpp-qt --target prod .

docker.push: ## Push the docker image
	@docker push danielvazzz/rinha-2024-q1-cpp-qt