# Git from Scratch++: A Deep Dive into Git Internals with C++ & Web UI

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![CodeQL Advanced](https://github.com/Arsh-2811/Git-from-scratch-cpp/actions/workflows/codeql.yml/badge.svg)](https://github.com/Arsh-2811/Git-from-scratch-cpp/actions/workflows/codeql.yml)
[![Node.js CI](https://github.com/Arsh-2811/Git-from-scratch-cpp/actions/workflows/node.js.yml/badge.svg)](https://github.com/Arsh-2811/Git-from-scratch-cpp/actions/workflows/node.js.yml)

Welcome to **Git from Scratch++**! This project is an ambitious exploration into the core mechanics of the Git version control system. It features:

1.  **`mygit`**: A custom Git implementation written from the ground up in **C++**, focusing on understanding and replicating Git's fundamental object model, indexing, branching, and merging logic.
2.  **Backend API**: A **Java Spring Boot** application that acts as a bridge, executing the `mygit` C++ command-line tool and exposing its functionality through a RESTful API.
3.  **Frontend UI**: A **React** single-page application providing a web-based interface to interact with `mygit` repositories via the backend API, visualizing branches, commits, and file contents.

**The primary goal is educational:** to demystify Git's internals by building a working subset of its features. It's a hands-on journey into blobs, trees, commits, the index, refs, and more.

## Project Vision

Git is ubiquitous, yet its internal workings can seem opaque. This project aims to peel back the layers by:

*   **Implementing Core Logic:** Rebuilding essential Git commands in C++ forces a deep understanding of the underlying data structures and algorithms.
*   **Providing Accessible Interaction:** Wrapping the C++ core with a standard REST API (Java/Spring Boot) makes it usable by various clients.
*   **Visualizing Git Concepts:** Offering a web UI (React) helps visualize commit history, branches, and repository structure, making abstract concepts more tangible.

Think of it as building your own functional model of Git to truly appreciate how the real thing works!

## Architecture Overview

The project follows a clear separation of concerns, with interactions flowing as follows:

1.  **Browser (User Interaction):** The user interacts with the **Frontend (React SPA)** loaded in their web browser.
    *   *Example Action: Clicking "View Commits" for the 'main' branch.*

2.  **Frontend (React SPA):** Translates user actions into API requests to the backend.
    *   *Sends Request: `GET /api/repos/my-repo/commits?ref=main`*

3.  **Backend (Java Spring Boot):** Receives the API request, identifies the target repository, and determines the appropriate `mygit` command.
    *   *Determines Command: `mygit log main` for the specified repository path.*

4.  **Backend executes `mygit`:** Runs the compiled **`mygit` C++ Executable** as a separate operating system process.
    *   *Process Call: `/path/to/mygit log main --repo-path /path/to/repos/my-repo`*

5.  **`mygit` C++ Executable:** Performs the requested Git operation by directly reading from and writing to the **Git Repository on Disk (`.mygit` directory)**.
    *   *Action: Reads commit objects, follows parent pointers.*

6.  **`mygit` produces output:** Prints results (like commit details) to standard output (`stdout`) and errors to standard error (`stderr`).
    *   *Output: Lines of text representing the commit log.*

7.  **Backend captures output:** Reads the `stdout` and `stderr` streams from the completed `mygit` process.

8.  **Backend parses output:** Converts the raw text output into structured Java objects (DTOs).
    *   *Transformation: Creates `List<CommitInfo>` objects from the log text.*

9.  **Backend sends response:** Serializes the Java DTOs into JSON and sends them back to the frontend as the HTTP response body.
    *   *Sends Response: `HTTP 200 OK` with JSON payload containing commit data.*

10. **Frontend renders UI:** Receives the JSON data and updates the user interface to display the information (e.g., the commit list).
    *   *Updates View: Shows the commit history to the user.*

*(This flow ensures that the core Git logic remains encapsulated within the C++ executable, while the Java backend handles API exposure and process management, and the React frontend provides the user interface.)*

## Features

### `mygit` (C++ Core) Implemented Commands:

*(See `mygitImplementation/README.md` for details)*

| Command          | Description                                                                    | Status      |
| :--------------- | :----------------------------------------------------------------------------- | :---------- |
| `init`           | Create/reinitialize an empty repository (`.mygit` directory)                   | Implemented |
| `add`            | Add file contents to the index (staging area)                                  | Implemented |
| `rm`             | Remove files from the index and optionally working directory                   | Implemented |
| `commit`         | Record changes (staged in the index) to the repository                       | Implemented |
| `status`         | Show the working tree status (changes vs index vs HEAD)                        | Implemented |
| `log`            | Show commit logs (linear history and `--graph` DOT output)                     | Implemented |
| `branch`         | List and create branches                                                       | Implemented |
| `checkout`       | Switch branches or restore working tree files (switch/detach HEAD)             | Implemented |
| `tag`            | List and create lightweight/annotated tags                                     | Implemented |
| `merge`          | Merge branches (fast-forward and basic 3-way merge with conflict detection)    | Implemented |
| `write-tree`     | Create a tree object from the current index                                    | Implemented |
| `read-tree`      | Read tree information into the index                                           | Implemented |
| `cat-file`       | Inspect Git objects (type, size, content)                                      | Implemented |
| `hash-object`    | Compute object ID and optionally create blob from file                         | Implemented |
| `rev-parse`      | Resolve ref names (branch, tag, HEAD, SHA) to full SHA-1                       | Implemented |
| `ls-tree`        | List the contents of a tree object                                             | Implemented |

### Web Interface Features:

*   List available local `mygit` repositories.
*   View commit history (linear or graph view) for any branch/ref.
*   Inspect details of individual commits (author, date, message, changes - *diff view pending*).
*   Browse the file tree at any given commit/ref.
*   View the content of individual files (blobs) at any commit/ref.
*   List branches and tags with their corresponding commit SHAs.
*   Basic object inspection (resolve refs, get object type/size).

## Technology Stack

| Component           | Technologies                                           |
| :------------------ | :----------------------------------------------------- |
| **Core Git Logic**  | C++ (C++17), CMake, zlib (for object compression)      |
| **Backend API**     | Java (17+), Spring Boot 3.x, Maven, Jackson (JSON)     |
| **Frontend UI**     | JavaScript, React 18.x, CSS, Fetch API (or Axios)      |
| **Development**     | Git (ironically!), VS Code / IntelliJ / CLion, Terminal |

## Setup and Running

**(Note:** Ensure you have a C++ compiler (like g++ or clang), CMake, Java JDK (17+), Maven, and Node.js/npm installed.)

1.  **Build the C++ `mygit` Executable:**
    ```bash
    cd mygitImplementation
    cmake -S . -B build
    cmake --build build
    # The executable will be in mygitImplementation/build/src/mygit (or similar)
    cd ..
    ```
    *(See `mygitImplementation/README.md` for more details)*

2.  **Run the Backend API:**
    *   **Crucially:** The backend needs to know where the `mygit` executable is and where your repositories are stored. This is configured in `backend/src/main/resources/application.properties`. Update these paths:
        ```properties
        # backend/src/main/resources/application.properties
        mygit.executable.path=/path/to/your/Git-from-scratch-cpp/mygitImplementation/build/src/mygit
        repository.base.path=/path/to/where/you/want/to/store/repos
        ```
    *   Run the Spring Boot application:
        ```bash
        cd backend
        mvn clean install
        mvn spring-boot:run
        # The API will typically be available at http://localhost:8080
        cd ..
        ```

3.  **Run the Frontend UI:**
    ```bash
    cd frontend
    npm install
    npm start
    # The UI will typically open automatically at http://localhost:3000
    cd ..
    ```

4.  **Usage:**
    *   Open `http://localhost:3000` in your browser.
    *   The UI will communicate with the backend API (`http://localhost:8080`).
    *   You can create/clone repositories *manually* using the `mygit` executable within the `repository.base.path` you configured for the backend. The UI currently focuses on *viewing* existing repositories managed by `mygit`.

## Future Work / Roadmap

*   Implement `mygit diff` and display diffs in the UI.
*   Implement `mygit clone`, `fetch`, `push` (much more complex, involves networking).
*   Add repository creation/initialization via the API/UI.
*   Implement staging/unstaging actions (`add`/`rm --cached`) in the UI.
*   Implement committing changes via the UI.
*   Improve merge conflict resolution visualization/handling.
*   More robust error handling and reporting in the UI.
*   Add user authentication/authorization (if needed).
*   Refactor C++ code for better performance/maintainability.
*   Add more comprehensive tests for both C++ and Java components.

## Contributing

Contributions are welcome! If you'd like to help improve the project, please feel free to fork the repository, make your changes, and submit a pull request. For major changes, please open an issue first to discuss.

## License

This project is licensed under the MIT License.
