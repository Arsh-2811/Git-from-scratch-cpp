# MyGit Web API Backend

This Spring Boot application serves as the backend API layer for the [Git from Scratch++](../README.md) project. Its primary role is to wrap the `mygit` C++ command-line executable and expose its functionality via a RESTful interface for consumption by the frontend or other clients.

## Purpose

*   Provides HTTP endpoints to interact with `mygit` repositories.
*   Executes the compiled `mygit` C++ program as a sub-process.
*   Parses the plain text output (`stdout`) and errors (`stderr`) from `mygit`.
*   Transforms the parsed data into structured JSON responses using DTOs (Data Transfer Objects).
*   Manages repository locations and basic error handling.

## Core Functionality

The backend doesn't reimplement any Git logic itself. It orchestrates calls to the `mygit` executable.

1.  **Request:** Receives an HTTP request (e.g., `GET /api/repos/my-repo/commits?ref=dev`).
2.  **Execution:** Locates the target repository (`my-repo`), determines the corresponding `mygit` command (`mygit log dev`), and executes the C++ program (`/path/to/mygit log dev --repo-path /path/to/repos/my-repo`).
3.  **Parsing:** Captures the standard output of the `mygit` process.
4.  **Transformation:** Parses the text output (e.g., commit log lines) into Java objects (`List<CommitInfo>`).
5.  **Response:** Serializes the Java objects into JSON and sends the HTTP response.

## API Endpoints

The API is structured around common Git concepts. Key controllers include:

| Controller                   | Base Path                          | Description                                        | Example `mygit` Command(s) Used                     |
| :--------------------------- | :--------------------------------- | :------------------------------------------------- | :---------------------------------------------------- |
| `RepositoryController`       | `/api/repos`                       | List available repositories                        | (Scans configured directory)                          |
| `BranchController`           | `/api/repos/{repoName}/branches`   | List branches                                      | `mygit branch`, `mygit rev-parse`                     |
| `TagController`              | `/api/repos/{repoName}/tags`       | List tags                                          | `mygit tag`, `mygit cat-file`                         |
| `CommitController`           | `/api/repos/{repoName}/commits`    | Get commit history (linear or graph)               | `mygit log`, `mygit log --graph`                      |
| `CommitController`           | `/api/repos/{repoName}/commits/{sha}`| Get details of a single commit                   | `mygit cat-file -p`                                   |
| `CodeViewController` (Tree)  | `/api/repos/{repoName}/tree/**`    | List files/directories in a tree                   | `mygit ls-tree`                                       |
| `CodeViewController` (Blob)  | `/api/repos/{repoName}/blob/**`    | Get raw content of a file (blob)                 | `mygit ls-tree`, `mygit cat-file -p` (indirectly)     |
| `ObjectInspectionController` | `/api/repos/{repoName}/objects/*`  | Resolve refs, get object type/size               | `mygit rev-parse`, `mygit cat-file -t`, `mygit cat-file -s` |

*(Refer to the Javadoc comments within the controller classes for detailed endpoint specifications, parameters, and return types).*

## Technologies

*   Java 17+
*   Spring Boot 3.x
    *   Spring Web (MVC)
    *   Spring Boot Actuator (optional, for monitoring)
*   Maven (Build Tool)
*   Jackson (JSON serialization/deserialization)

## Setup and Running

**Prerequisites:**

*   Java Development Kit (JDK) 17 or later.
*   Apache Maven.
*   A compiled `mygit` executable (see [mygitImplementation/README.md](../mygitImplementation/README.md)).

**Configuration:**

1.  **Edit `src/main/resources/application.properties`:**
    *   Set `mygit.executable.path`: Absolute path to your compiled `mygit` executable.
    *   Set `repository.base.path`: Absolute path to the directory where your `mygit` repositories are (or will be) stored. The backend will look for repositories *inside* this base path.

**Running:**

1.  **Navigate to this directory:**
    ```bash
    cd /path/to/Git-from-scratch-cpp/backend
    ```
2.  **Run using Maven:**
    ```bash
    mvn clean install
    mvn spring-boot:run
    ```
3.  The API will typically be available at `http://localhost:8080`.

## Key Components

*   **`controller/`**: Defines REST API endpoints using Spring MVC annotations. Delegates logic to services.
*   **`service/`**: Contains business logic.
    *   `GitCommandExecutor`: Handles the execution of the external `mygit` C++ process.
    *   `OutputParserService`: Parses the raw string output from `mygit` into DTOs.
    *   `ViewService`: Orchestrates calls to the executor and parser to fulfill controller requests.
*   **`dto/`**: Data Transfer Objects used for API request/response bodies (e.g., `CommitInfo`, `BranchInfo`, `FileInfo`, `GraphData`).
*   **`config/`**: Application configuration (e.g., `WebConfig` for CORS).
*   **`resources/application.properties`**: External configuration for paths, server port, etc.

## Error Handling

*   The `ViewService` and `GitCommandExecutor` attempt to catch errors from the `mygit` process (e.g., non-zero exit codes, error messages on stderr).
*   Exceptions (like `IllegalArgumentException` for bad input or `IOException` for process errors) are caught in the controllers.
*   Controllers typically map exceptions to appropriate HTTP status codes (e.g., 400 Bad Request, 404 Not Found, 500 Internal Server Error). A global exception handler (`@ControllerAdvice`) could be added for more centralized error management.