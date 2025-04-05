**Important Notes:**

1.  **DTOs (Data Transfer Objects):** You'll need to create simple Java classes (POJOs) to represent the structure of request bodies and response bodies (e.g., `CommitInfo`, `FileInfo`, `BranchInfo`, `StatusReport`, `ApiError`). I'll mention them but won't define every single one here.
2.  **Error Handling:** The examples use basic `ResponseEntity` but you should implement proper exception handling (e.g., using `@ControllerAdvice`) to return consistent error responses (e.g., a standard `ApiError` JSON object).
3.  **Parsing:** The biggest task within each implementation will be **executing the `mygit` command** using `ProcessBuilder` and **parsing its plain text output** into the structured JSON format defined by the return types. Consider if `mygit` could optionally output JSON directly (`--json` flag?) to simplify this.
4.  **Security:** **Crucially important!**
    *   **Path Traversal:** Validate `repoName` and any file paths received to ensure they stay within your designated base repository directory. Never allow `../` or absolute paths from user input to form the execution path.
    *   **Command Injection:** Sanitize *all* inputs used in command arguments (commit messages, branch names, file paths, refs). Use `ProcessBuilder` correctly, passing arguments individually, not constructing a single command string.
5.  **Repository Path:** Assume you have a service (`RepositoryService` or similar) that can resolve a `repoName` into its full, validated path on the server.

---

**Directory Structure (Conceptual within your Spring Boot project):**

```
src/main/java/com/yourcompany/mygitapp/
├── controller/
│   ├── RepositoryController.java
│   ├── CodeViewController.java      // For browsing files/trees
│   ├── CommitController.java
│   ├── BranchController.java
│   ├── TagController.java
│   ├── StatusController.java        // Handles status, maybe index ops too
│   ├── OperationController.java     // Checkout, Merge
│   └── ObjectInspectionController.java // cat-file, rev-parse etc.
├── dto/
│   ├── CreateRepoRequest.java
│   ├── CommitRequest.java
│   ├── BranchCreateRequest.java
│   ├── TagCreateRequest.java
│   ├── CheckoutRequest.java
│   ├── MergeRequest.java
│   ├── CommitInfo.java
│   ├── FileInfo.java            // For ls-tree output
│   ├── BranchInfo.java
│   ├── TagInfo.java
│   ├── StatusReport.java        // Parsed output of mygit status
│   ├── ObjectInfo.java          // For cat-file / rev-parse
│   └── ApiError.java            // Standard error response
├── service/
│   ├── RepositoryService.java     // Locates repos, executes mygit commands
│   └── GitCommandExecutor.java  // Helper for ProcessBuilder logic
│   └── OutputParserService.java   // Logic to parse mygit stdout/stderr
└── ... (Application.java, etc.)
```

---

**1. `RepositoryController.java`**

Handles overall repository management.

```java
package com.yourcompany.mygitapp.controller;

import com.yourcompany.mygitapp.dto.*; // Import your DTOs
import com.yourcompany.mygitapp.service.RepositoryService; // Your service layer
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.List;

@RestController
@RequestMapping("/api/repos")
// Consider adding @CrossOrigin if React is on a different port
public class RepositoryController {

    @Autowired
    private RepositoryService repositoryService; // Inject your service

    /**
     * @Description: Lists all available 'mygit' repositories managed by the application.
     * @HTTPMethod: GET
     * @Path: /api/repos
     * @Parameters: None
     * @ReturnType: List<String> - A list of repository names.
     * @MygitCommand: N/A (Backend scans a directory) or potentially custom logic.
     * @SampleOutput: ["project-alpha", "my-library", "dotfiles"]
     */
    @GetMapping
    public ResponseEntity<List<String>> getAllRepositories() {
        try {
            // This assumes your service scans a base directory
            List<String> repoNames = repositoryService.findAllRepositoryNames();
            return ResponseEntity.ok(repoNames);
        } catch (Exception e) {
            // Log error
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).body(null); // Or return ApiError DTO
        }
    }

    /**
     * @Description: Creates a new empty 'mygit' repository.
     * @HTTPMethod: POST
     * @Path: /api/repos
     * @Parameters: Request Body (JSON) - e.g., {"name": "new-project"} (Use a CreateRepoRequest DTO)
     * @ReturnType: Void or ResponseEntity<Void> on success.
     * @MygitCommand: `mygit init` (executed in a new directory named after the repo)
     * @SampleInputBody: { "name": "new-project" }
     * @SampleOutput: HTTP 201 Created (on success), HTTP 400/500 on error.
     */
    @PostMapping
    public ResponseEntity<Void> createRepository(@RequestBody CreateRepoRequest request) {
        try {
            // !! Add validation for request.getName() (prevent '..', '/', etc.) !!
            boolean success = repositoryService.initializeRepository(request.getName());
            if (success) {
                return ResponseEntity.status(HttpStatus.CREATED).build();
            } else {
                // Maybe repository already exists or other init error
                return ResponseEntity.status(HttpStatus.BAD_REQUEST).build(); // Or more specific error
            }
        } catch (IllegalArgumentException e) { // Example validation error
             return ResponseEntity.status(HttpStatus.BAD_REQUEST).build();
        } catch (Exception e) {
            // Log error
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).build();
        }
    }

    // Add DELETE /api/repos/{repoName} later if needed
}
```

---

**2. `CodeViewController.java`**

Handles browsing files and directory structures within a repository.

```java
package com.yourcompany.mygitapp.controller;

import com.yourcompany.mygitapp.dto.*;
import com.yourcompany.mygitapp.service.RepositoryService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.List;

@RestController
@RequestMapping("/api/repos/{repoName}/tree")
public class CodeViewController {

    @Autowired
    private RepositoryService repositoryService;

    /**
     * @Description: Lists the contents (files and directories) of a specific tree object
     *              (defaults to the tree of the current HEAD or specified ref/commit/tree-sha).
     * @HTTPMethod: GET
     * @Path: /api/repos/{repoName}/tree
     * @Path: /api/repos/{repoName}/tree/{*path} (To view subdirectories)
     * @PathVariables:
     *      - repoName: The name of the repository.
     *      - path (optional): The subdirectory path within the repo.
     * @QueryParameters:
     *      - ref (optional): Branch name, tag name, or commit SHA to view (defaults to HEAD).
     *      - recursive (optional, boolean): Corresponds to `ls-tree -r`. Defaults to false.
     * @ReturnType: List<FileInfo> - List of objects describing files/trees (mode, type, sha, name).
     * @MygitCommand: `mygit ls-tree [-r] <tree-ish>` where <tree-ish> is resolved from `ref` and `path`.
     *                Example: `mygit ls-tree HEAD:src/main` or `mygit ls-tree my-branch`
     * @SampleOutput: [ {"mode": "100644", "type": "blob", "sha": "abc...", "name": "README.md"},
     *                  {"mode": "040000", "type": "tree", "sha": "def...", "name": "src"} ]
     */
    @GetMapping(value = {"", "/{**path}"}) // Capture optional path including slashes
    public ResponseEntity<List<FileInfo>> listTreeContents(
            @PathVariable String repoName,
            @PathVariable(required = false) String path,
            @RequestParam(required = false, defaultValue = "HEAD") String ref,
            @RequestParam(required = false, defaultValue = "false") boolean recursive) {
        try {
            // !! Validate repoName, path, ref !!
            // Construct the <tree-ish> argument carefully based on ref and path
            // e.g., ref + (path != null ? ":" + path : "")
            String treeIsh = repositoryService.buildTreeIsh(ref, path);

            List<FileInfo> contents = repositoryService.lsTree(repoName, treeIsh, recursive);
            return ResponseEntity.ok(contents);
        } catch (Exception e) {
            // Log error, return appropriate status (404 if repo/ref/path not found?)
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).body(null);
        }
    }

     /**
     * @Description: Gets the raw content of a specific file (blob) at a given ref.
     * @HTTPMethod: GET
     * @Path: /api/repos/{repoName}/blob/{*filePath}
     * @PathVariables:
     *      - repoName: The name of the repository.
     *      - filePath: The full path to the file within the repo.
     * @QueryParameters:
     *      - ref (optional): Branch name, tag name, or commit SHA (defaults to HEAD).
     * @ReturnType: String (Raw file content). Consider `ResponseEntity<byte[]>` for binary files.
     *              Set appropriate Content-Type header.
     * @MygitCommand: `mygit cat-file -p <ref>:<filePath>` (Requires resolving ref:path to blob SHA first using ls-tree or rev-parse maybe)
     *                Alternatively, if `cat-file` supports `<ref>:<path>` directly. If not:
     *                1. `mygit ls-tree <ref> <filePath>` -> get blob SHA
     *                2. `mygit cat-file -p <blob_sha>`
     * @SampleOutput: (Raw text content of the file)
     */
    @GetMapping("/../blob/{**filePath}") // Use ../blob to avoid conflict with /tree path
    public ResponseEntity<String> getFileContent(
            @PathVariable String repoName,
            @PathVariable String filePath,
            @RequestParam(required = false, defaultValue = "HEAD") String ref) {
         try {
            // !! Validate repoName, filePath, ref !!
            // Resolve ref:filePath to the blob SHA if needed
            // Execute cat-file -p
            String content = repositoryService.getFileContent(repoName, ref, filePath);
            // Determine content type (text/plain, etc.) - maybe basic detection
            return ResponseEntity.ok().contentType(org.springframework.http.MediaType.TEXT_PLAIN).body(content);
         } catch (Exception e) {
             // 404 if not found
             return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).body(null);
         }
    }
}
```

---

**3. `CommitController.java`**

Handles commit history and related actions.

```java
package com.yourcompany.mygitapp.controller;

import com.yourcompany.mygitapp.dto.*;
import com.yourcompany.mygitapp.service.RepositoryService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.List; // Or a more complex graph structure DTO

@RestController
@RequestMapping("/api/repos/{repoName}/commits")
public class CommitController {

    @Autowired
    private RepositoryService repositoryService;

    /**
     * @Description: Retrieves the commit history for the repository, potentially for a specific ref.
     * @HTTPMethod: GET
     * @Path: /api/repos/{repoName}/commits
     * @PathVariables:
     *      - repoName: The name of the repository.
     * @QueryParameters:
     *      - ref (optional): Branch/tag/commit SHA to start the log from (defaults to HEAD).
     *      - graph (optional, boolean): If true, request graph structure (requires complex parsing). Defaults to false.
     *      - limit (optional, int): Limit the number of commits returned.
     *      - skip (optional, int): Skip the first N commits (for pagination).
     * @ReturnType: List<CommitInfo> (or a specific Graph DTO if graph=true). CommitInfo includes SHA, author, date, message.
     * @MygitCommand: `mygit log [--graph] [<ref>]` (Add pagination logic if needed)
     * @SampleOutput (graph=false): [ {"sha": "...", "author": "...", "date": "...", "message": "..."}, ... ]
     * @SampleOutput (graph=true): (A more complex JSON structure representing nodes and edges)
     */
    @GetMapping
    public ResponseEntity<?> getCommitLog( // Return type varies based on 'graph'
            @PathVariable String repoName,
            @RequestParam(required = false, defaultValue = "HEAD") String ref,
            @RequestParam(required = false, defaultValue = "false") boolean graph,
            @RequestParam(required = false) Integer limit,
            @RequestParam(required = false) Integer skip) {
        try {
            // !! Validate repoName, ref !!
            if (graph) {
                 // Call service method that runs `mygit log --graph` and parses it
                 Object graphData = repositoryService.getCommitGraph(repoName, ref, limit, skip);
                 return ResponseEntity.ok(graphData);
            } else {
                 // Call service method that runs `mygit log` and parses it
                 List<CommitInfo> commits = repositoryService.getCommitHistory(repoName, ref, limit, skip);
                 return ResponseEntity.ok(commits);
            }
        } catch (Exception e) {
            // Log error
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).body(null);
        }
    }

    /**
     * @Description: Creates a new commit with changes currently staged in the index.
     * @HTTPMethod: POST
     * @Path: /api/repos/{repoName}/commits
     * @PathVariables:
     *      - repoName: The name of the repository.
     * @Parameters: Request Body (JSON) - e.g., {"message": "Fix bug #123"} (Use CommitRequest DTO). Assume author/committer info is configured globally in mygit or handled server-side.
     * @ReturnType: CommitInfo (Details of the newly created commit) or ResponseEntity<Void>.
     * @MygitCommand: `mygit commit -m "<message>"`
     * @SampleInputBody: { "message": "Implement feature X" }
     * @SampleOutput: HTTP 201 Created with CommitInfo body, or HTTP 400/500 on error.
     */
    @PostMapping
    public ResponseEntity<CommitInfo> createCommit(
            @PathVariable String repoName,
            @RequestBody CommitRequest request) {
        try {
            // !! Validate repoName, request.getMessage() !! Sanitize message!
            // Execute commit command
            CommitInfo newCommit = repositoryService.createCommit(repoName, request.getMessage());
             if (newCommit != null) {
                return ResponseEntity.status(HttpStatus.CREATED).body(newCommit);
            } else {
                // Handle cases like "nothing to commit" or commit errors
                return ResponseEntity.status(HttpStatus.BAD_REQUEST).build(); // Or other appropriate status
            }
        } catch (IllegalArgumentException e) {
            return ResponseEntity.status(HttpStatus.BAD_REQUEST).build();
        } catch (Exception e) {
            // Log error
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).build();
        }
    }

     /**
     * @Description: Gets details for a single commit.
     * @HTTPMethod: GET
     * @Path: /api/repos/{repoName}/commits/{commitSha}
     * @PathVariables:
     *      - repoName: The name of the repository.
     *      - commitSha: The SHA of the commit to inspect.
     * @ReturnType: CommitInfo (or a more detailed version including changes if needed).
     * @MygitCommand: `mygit cat-file -p <commitSha>` (and parse output) or maybe `mygit log -1 <commitSha>`
     * @SampleOutput: { "sha": "...", "author": "...", "date": "...", "message": "...", "tree": "...", "parents": ["..."] }
     */
    @GetMapping("/{commitSha}")
    public ResponseEntity<CommitInfo> getCommitDetails(
        @PathVariable String repoName,
        @PathVariable String commitSha) {
         try {
            // !! Validate repoName, commitSha !!
            CommitInfo commitDetails = repositoryService.getCommitDetails(repoName, commitSha);
            if (commitDetails != null) {
                return ResponseEntity.ok(commitDetails);
            } else {
                return ResponseEntity.notFound().build();
            }
         } catch (Exception e) {
             // Log error
             return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).body(null);
         }
    }
}
```

---

**4. `BranchController.java`**

Manages branches within a repository.

```java
package com.yourcompany.mygitapp.controller;

import com.yourcompany.mygitapp.dto.*;
import com.yourcompany.mygitapp.service.RepositoryService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.List;

@RestController
@RequestMapping("/api/repos/{repoName}/branches")
public class BranchController {

    @Autowired
    private RepositoryService repositoryService;

    /**
     * @Description: Lists all branches in the repository.
     * @HTTPMethod: GET
     * @Path: /api/repos/{repoName}/branches
     * @PathVariables:
     *      - repoName: The name of the repository.
     * @ReturnType: List<BranchInfo> - List includes branch name and possibly the SHA it points to. Indicate current branch.
     * @MygitCommand: `mygit branch` (and parse output, identifying '*' for current branch)
     * @SampleOutput: [ {"name": "main", "sha": "...", "isCurrent": true}, {"name": "develop", "sha": "...", "isCurrent": false} ]
     */
    @GetMapping
    public ResponseEntity<List<BranchInfo>> listBranches(@PathVariable String repoName) {
        try {
            // !! Validate repoName !!
            List<BranchInfo> branches = repositoryService.listBranches(repoName);
            return ResponseEntity.ok(branches);
        } catch (Exception e) {
            // Log error
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).body(null);
        }
    }

    /**
     * @Description: Creates a new branch.
     * @HTTPMethod: POST
     * @Path: /api/repos/{repoName}/branches
     * @PathVariables:
     *      - repoName: The name of the repository.
     * @Parameters: Request Body (JSON) - e.g., {"name": "feature-xyz", "startPoint": "main"} (Use BranchCreateRequest DTO). startPoint is optional.
     * @ReturnType: BranchInfo (Details of the new branch) or ResponseEntity<Void>.
     * @MygitCommand: `mygit branch <name> [<start>]`
     * @SampleInputBody: { "name": "new-feature", "startPoint": "develop" }
     * @SampleOutput: HTTP 201 Created with BranchInfo, or HTTP 400/500 on error.
     */
    @PostMapping
    public ResponseEntity<BranchInfo> createBranch(
            @PathVariable String repoName,
            @RequestBody BranchCreateRequest request) {
        try {
            // !! Validate repoName, request.getName(), request.getStartPoint() !! Sanitize names!
            BranchInfo newBranch = repositoryService.createBranch(repoName, request.getName(), request.getStartPoint());
             if (newBranch != null) {
                 return ResponseEntity.status(HttpStatus.CREATED).body(newBranch);
             } else {
                 // Handle errors like branch already exists, invalid startPoint
                 return ResponseEntity.status(HttpStatus.BAD_REQUEST).build();
             }
        } catch (IllegalArgumentException e) {
            return ResponseEntity.status(HttpStatus.BAD_REQUEST).build();
        } catch (Exception e) {
            // Log error
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).build();
        }
    }

    // Add DELETE /api/repos/{repoName}/branches/{branchName} later when `mygit branch -d` is implemented.
    // Remember to URL-encode branch names if they contain special characters like '/'.
}
```

---

**5. `TagController.java`**

Manages tags within a repository.

```java
package com.yourcompany.mygitapp.controller;

import com.yourcompany.mygitapp.dto.*;
import com.yourcompany.mygitapp.service.RepositoryService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.List;

@RestController
@RequestMapping("/api/repos/{repoName}/tags")
public class TagController {

    @Autowired
    private RepositoryService repositoryService;

    /**
     * @Description: Lists all tags in the repository.
     * @HTTPMethod: GET
     * @Path: /api/repos/{repoName}/tags
     * @PathVariables:
     *      - repoName: The name of the repository.
     * @ReturnType: List<TagInfo> - List includes tag name and possibly the SHA it points to, and type (lightweight/annotated).
     * @MygitCommand: `mygit tag` (and parse output)
     * @SampleOutput: [ {"name": "v1.0", "sha": "...", "type": "lightweight"}, {"name": "v1.1-beta", "sha": "...", "type": "annotated"} ]
     */
    @GetMapping
    public ResponseEntity<List<TagInfo>> listTags(@PathVariable String repoName) {
        try {
            // !! Validate repoName !!
            List<TagInfo> tags = repositoryService.listTags(repoName);
            return ResponseEntity.ok(tags);
        } catch (Exception e) {
            // Log error
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).body(null);
        }
    }

   /**
     * @Description: Creates a new tag (lightweight or annotated).
     * @HTTPMethod: POST
     * @Path: /api/repos/{repoName}/tags
     * @PathVariables:
     *      - repoName: The name of the repository.
     * @Parameters: Request Body (JSON) - e.g., {"name": "v2.0", "object": "HEAD", "annotate": true, "message": "Release version 2.0"} (Use TagCreateRequest DTO). object, annotate, message are optional.
     * @ReturnType: TagInfo (Details of the new tag) or ResponseEntity<Void>.
     * @MygitCommand: `mygit tag <name> [<obj>]` or `mygit tag -a -m "<msg>" <name> [<obj>]`
     * @SampleInputBody: { "name": "v1.0-final", "annotate": true, "message": "Final release for 1.0" }
     * @SampleOutput: HTTP 201 Created with TagInfo, or HTTP 400/500 on error.
     */
    @PostMapping
    public ResponseEntity<TagInfo> createTag(
            @PathVariable String repoName,
            @RequestBody TagCreateRequest request) {
        try {
            // !! Validate repoName, request parameters !! Sanitize names/messages!
            TagInfo newTag = repositoryService.createTag(
                repoName,
                request.getName(),
                request.getObject(), // Optional target object (commit sha, etc.) defaults usually to HEAD
                request.isAnnotate(),
                request.getMessage() // Optional message for annotated tags
            );
             if (newTag != null) {
                 return ResponseEntity.status(HttpStatus.CREATED).body(newTag);
             } else {
                 // Handle errors like tag already exists, invalid object
                 return ResponseEntity.status(HttpStatus.BAD_REQUEST).build();
             }
        } catch (IllegalArgumentException e) {
             return ResponseEntity.status(HttpStatus.BAD_REQUEST).build();
        } catch (Exception e) {
            // Log error
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).build();
        }
    }

    // Add DELETE /api/repos/{repoName}/tags/{tagName} later if needed (`mygit tag -d`)
}
```

---

**6. `StatusController.java`**

Handles reporting status and managing the index/staging area.

```java
package com.yourcompany.mygitapp.controller;

import com.yourcompany.mygitapp.dto.*;
import com.yourcompany.mygitapp.service.RepositoryService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.List; // For file lists in request/response

@RestController
@RequestMapping("/api/repos/{repoName}/status")
public class StatusController {

    @Autowired
    private RepositoryService repositoryService;

    /**
     * @Description: Shows the working tree status (staged, unstaged, untracked files).
     * @HTTPMethod: GET
     * @Path: /api/repos/{repoName}/status
     * @PathVariables:
     *      - repoName: The name of the repository.
     * @ReturnType: StatusReport - A DTO containing lists of files for each category (staged, modified, untracked, etc.) and the current branch.
     * @MygitCommand: `mygit status` (and parse output extensively)
     * @SampleOutput: { "currentBranch": "main", "staged": ["file1.txt"], "modified": ["file2.js"], "untracked": ["new_dir/"] }
     */
    @GetMapping
    public ResponseEntity<StatusReport> getStatus(@PathVariable String repoName) {
        try {
            // !! Validate repoName !!
            StatusReport report = repositoryService.getStatus(repoName);
            return ResponseEntity.ok(report);
        } catch (Exception e) {
            // Log error
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).body(null);
        }
    }

    /**
     * @Description: Adds file contents to the index (stages changes).
     * @HTTPMethod: POST
     * @Path: /api/repos/{repoName}/index/files // Using sub-resource 'index'
     * @PathVariables:
     *      - repoName: The name of the repository.
     * @Parameters: Request Body (JSON) - e.g., {"files": ["src/main.c", "README.md"]} or {"files": ["."]} to add all.
     * @ReturnType: ResponseEntity<Void>
     * @MygitCommand: `mygit add <file>...`
     * @SampleInputBody: { "files": ["file1.txt", "docs/guide.md"] }
     * @SampleOutput: HTTP 200 OK (on success), HTTP 400/500 on error.
     */
    @PostMapping("/../index/files") // Route relative to repo
    public ResponseEntity<Void> addFilesToIndex(
            @PathVariable String repoName,
            @RequestBody FileListRequest request) { // FileListRequest DTO contains List<String> files
        try {
            // !! Validate repoName and ALL file paths in request.getFiles() !! Prevent '..' etc.
            boolean success = repositoryService.addFiles(repoName, request.getFiles());
            if (success) {
                return ResponseEntity.ok().build();
            } else {
                return ResponseEntity.status(HttpStatus.BAD_REQUEST).build(); // Or specific error
            }
        } catch (IllegalArgumentException e) {
             return ResponseEntity.status(HttpStatus.BAD_REQUEST).build();
        } catch (Exception e) {
            // Log error
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).build();
        }
    }

     /**
     * @Description: Removes files from the index (unstages changes). Does not touch working directory.
     * @HTTPMethod: DELETE
     * @Path: /api/repos/{repoName}/index/files
     * @PathVariables:
     *      - repoName: The name of the repository.
     * @QueryParameters:
     *      - path: The file path to unstage (required). Repeat query param for multiple files if needed, or accept a list in body (less RESTful for DELETE). Let's use one path param for simplicity here.
     * @ReturnType: ResponseEntity<Void>
     * @MygitCommand: `mygit rm --cached <file>`
     * @SampleInputQuery: ?path=src/removed.java
     * @SampleOutput: HTTP 200 OK (on success), HTTP 400/500 on error.
     */
    @DeleteMapping("/../index/files") // Route relative to repo
    public ResponseEntity<Void> removeFileFromIndex(
            @PathVariable String repoName,
            @RequestParam String path) {
        try {
            // !! Validate repoName and path !!
            boolean success = repositoryService.removeFileCached(repoName, path);
             if (success) {
                return ResponseEntity.ok().build();
            } else {
                return ResponseEntity.status(HttpStatus.BAD_REQUEST).build(); // Or specific error
            }
        } catch (IllegalArgumentException e) {
             return ResponseEntity.status(HttpStatus.BAD_REQUEST).build();
        } catch (Exception e) {
             // Log error
             return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).build();
        }
    }

    /**
     * @Description: Removes files from the working tree AND from the index.
     * @HTTPMethod: DELETE
     * @Path: /api/repos/{repoName}/workdir/files // Separate path for clarity
     * @PathVariables:
     *      - repoName: The name of the repository.
     * @QueryParameters:
     *      - path: The file path to remove (required).
     * @ReturnType: ResponseEntity<Void>
     * @MygitCommand: `mygit rm <file>`
     * @SampleInputQuery: ?path=obsolete.txt
     * @SampleOutput: HTTP 200 OK (on success), HTTP 400/500 on error.
     */
    @DeleteMapping("/../workdir/files") // Route relative to repo
    public ResponseEntity<Void> removeFileFromWorkdir(
            @PathVariable String repoName,
            @RequestParam String path) {
        try {
            // !! Validate repoName and path !! This DELETES files on the server! Be careful!
            boolean success = repositoryService.removeFile(repoName, path);
             if (success) {
                return ResponseEntity.ok().build();
            } else {
                return ResponseEntity.status(HttpStatus.BAD_REQUEST).build(); // Or specific error
            }
        } catch (IllegalArgumentException e) {
             return ResponseEntity.status(HttpStatus.BAD_REQUEST).build();
        } catch (Exception e) {
             // Log error
             return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).build();
        }
    }
}
```

---

**7. `OperationController.java`**

Handles major state-changing operations like checkout and merge.

```java
package com.yourcompany.mygitapp.controller;

import com.yourcompany.mygitapp.dto.*;
import com.yourcompany.mygitapp.service.RepositoryService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

@RestController
@RequestMapping("/api/repos/{repoName}/ops") // Grouping operations
public class OperationController {

    @Autowired
    private RepositoryService repositoryService;

    /**
     * @Description: Switches the current branch or restores working tree files (checks out commit/ref).
     * @HTTPMethod: POST
     * @Path: /api/repos/{repoName}/ops/checkout
     * @PathVariables:
     *      - repoName: The name of the repository.
     * @Parameters: Request Body (JSON) - e.g., {"target": "develop"} or {"target": "abc123sha"} (Use CheckoutRequest DTO).
     * @ReturnType: ResponseEntity<Void> (or maybe return the new StatusReport).
     * @MygitCommand: `mygit checkout <branch|commit>`
     * @SampleInputBody: { "target": "feature/login" }
     * @SampleOutput: HTTP 200 OK (on success), HTTP 400/500 on error (e.g., local changes, invalid target).
     */
    @PostMapping("/checkout")
    public ResponseEntity<Void> checkout(
            @PathVariable String repoName,
            @RequestBody CheckoutRequest request) {
        try {
            // !! Validate repoName, request.getTarget() !! Sanitize target ref!
            boolean success = repositoryService.checkout(repoName, request.getTarget());
            if (success) {
                return ResponseEntity.ok().build();
            } else {
                // Checkout likely failed due to uncommitted changes or invalid target
                return ResponseEntity.status(HttpStatus.BAD_REQUEST).build(); // Or CONFLICT (409)
            }
        } catch (IllegalArgumentException e) {
            return ResponseEntity.status(HttpStatus.BAD_REQUEST).build();
        } catch (Exception e) {
            // Log error
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).build();
        }
    }

    /**
     * @Description: Merges another branch into the current branch.
     * @HTTPMethod: POST
     * @Path: /api/repos/{repoName}/ops/merge
     * @PathVariables:
     *      - repoName: The name of the repository.
     * @Parameters: Request Body (JSON) - e.g., {"branch": "feature/login"} (Use MergeRequest DTO).
     * @ReturnType: MergeResult DTO (indicating success, conflicts, fast-forward) or ResponseEntity<Void>.
     * @MygitCommand: `mygit merge <branch>`
     * @SampleInputBody: { "branch": "develop" }
     * @SampleOutput: HTTP 200 OK with MergeResult body (e.g., {"status": "Merged", "newHead": "..."}),
     *                HTTP 409 Conflict (if merge conflicts occur),
     *                HTTP 400/500 on other errors.
     */
    @PostMapping("/merge")
    public ResponseEntity<MergeResult> mergeBranch( // Define MergeResult DTO
            @PathVariable String repoName,
            @RequestBody MergeRequest request) {
        try {
            // !! Validate repoName, request.getBranch() !! Sanitize branch name!
            MergeResult result = repositoryService.merge(repoName, request.getBranch());

            if ("CONFLICT".equals(result.getStatus())) { // Assuming MergeResult has a status field
                 return ResponseEntity.status(HttpStatus.CONFLICT).body(result);
            } else if ("ERROR".equals(result.getStatus())) { // Generic error
                 return ResponseEntity.status(HttpStatus.BAD_REQUEST).body(result);
            } else { // Merged, Already up-to-date, etc.
                return ResponseEntity.ok(result);
            }
        } catch (IllegalArgumentException e) {
            // Return MergeResult indicating error?
            return ResponseEntity.status(HttpStatus.BAD_REQUEST).body(new MergeResult("ERROR", e.getMessage()));
        } catch (Exception e) {
            // Log error
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).body(new MergeResult("ERROR", "Internal server error"));
        }
    }
}
```

---

**8. `ObjectInspectionController.java`**

Handles low-level `mygit` commands for inspecting objects (less common for typical UI, but useful).

```java
package com.yourcompany.mygitapp.controller;

import com.yourcompany.mygitapp.dto.*;
import com.yourcompany.mygitapp.service.RepositoryService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpStatus;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

@RestController
@RequestMapping("/api/repos/{repoName}/objects")
public class ObjectInspectionController {

    @Autowired
    private RepositoryService repositoryService;

    /**
     * @Description: Resolves a ref name (branch, tag, HEAD, etc.) to its full SHA-1 hash.
     * @HTTPMethod: GET
     * @Path: /api/repos/{repoName}/objects/_resolve // Using _resolve to avoid clashing with SHA path
     * @PathVariables:
     *      - repoName: The name of the repository.
     * @QueryParameters:
     *      - ref: The reference name to resolve (required).
     * @ReturnType: String (The resolved SHA-1 hash).
     * @MygitCommand: `mygit rev-parse <ref>`
     * @SampleInputQuery: ?ref=HEAD
     * @SampleOutput: "a1b2c3d4e5f6..." (as plain text or simple JSON {"sha": "..."})
     */
    @GetMapping("/_resolve")
    public ResponseEntity<String> resolveReference(
            @PathVariable String repoName,
            @RequestParam String ref) {
        try {
            // !! Validate repoName, ref !! Sanitize ref!
            String sha = repositoryService.revParse(repoName, ref);
            if (sha != null) {
                 // Return as plain text for simplicity, or JSON
                return ResponseEntity.ok().contentType(MediaType.TEXT_PLAIN).body(sha);
            } else {
                return ResponseEntity.notFound().build();
            }
        } catch (IllegalArgumentException e) {
             return ResponseEntity.status(HttpStatus.BAD_REQUEST).build();
        } catch (Exception e) {
            // Log error
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).body(null);
        }
    }

    /**
     * @Description: Provides content, type, or size information for a repository object (commit, tree, blob, tag).
     * @HTTPMethod: GET
     * @Path: /api/repos/{repoName}/objects/{objectSha}
     * @PathVariables:
     *      - repoName: The name of the repository.
     *      - objectSha: The SHA-1 hash of the object.
     * @QueryParameters:
     *      - op: The operation type (required): 'p' (pretty-print content), 't' (type), 's' (size).
     * @ReturnType: String (Content, type name, or size as string). Set appropriate Content-Type.
     * @MygitCommand: `mygit cat-file -p <objectSha>` OR `mygit cat-file -t <objectSha>` OR `mygit cat-file -s <objectSha>`
     * @SampleInputQuery: ?op=t
     * @SampleOutput (op=t): "commit" (as plain text)
     * @SampleOutput (op=s): "172" (as plain text)
     * @SampleOutput (op=p): (Raw object content, e.g., commit details or file content, as plain text)
     */
    @GetMapping("/{objectSha}")
    public ResponseEntity<String> getObjectInfo(
            @PathVariable String repoName,
            @PathVariable String objectSha,
            @RequestParam String op) { // op should be 'p', 't', or 's'
        try {
            // !! Validate repoName, objectSha, op !!
            String result = repositoryService.catFile(repoName, objectSha, op);
             if (result != null) {
                // Consider setting Content-Type based on 'op' or object type if possible
                return ResponseEntity.ok().contentType(MediaType.TEXT_PLAIN).body(result);
            } else {
                return ResponseEntity.notFound().build();
            }
        } catch (IllegalArgumentException e) { // Invalid 'op' or SHA format
             return ResponseEntity.status(HttpStatus.BAD_REQUEST).build();
        } catch (Exception e) {
            // Log error
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).body(null);
        }
    }

    // `hash-object` is harder to expose via REST meaningfully without file uploads
    // or assuming files exist server-side, so maybe skip for now unless needed.
}