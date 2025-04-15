#!/bin/bash

DEFAULT_REPO_PATH="../repositories"
REPO_PATH=${1:-$DEFAULT_REPO_PATH}

java -DREPO_BASE_PATH="$REPO_PATH" -jar backend/backend-0.0.1-SNAPSHOT.jar &
SPRING_PID=$!

cleanup() {
  echo "Stopping processes..."
  kill $SPRING_PID
  kill $HTTP_SERVER_PID
}

trap cleanup SIGINT

cd frontend/build
npx http-server -p 3000 &
HTTP_SERVER_PID=$!

wait $SPRING_PID
wait $HTTP_SERVER_PID