package com.example.backend.service;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Optional;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;

import com.example.backend.dto.BranchInfo;
import com.example.backend.dto.CommitInfo;
import com.example.backend.dto.FileInfo;
import com.example.backend.dto.GraphData;
import com.example.backend.dto.TagInfo;

@Service
public class ViewService {
    @Value("${base.url}")
    private String repositoriesBasePath;

    @Autowired
    private GitCommandExecutor commandExecutor;

    @Autowired
    private OutputParserService parserService;

    private TagInfo getTagDetails(String repoName, String tagName) throws IOException, InterruptedException {
        // Reuse logic from listTags enhancement: resolve ref, cat-file -t, cat-file -p
        // if annotated
        // This is just a placeholder structure
        TagInfo tagInfo = new TagInfo();
        tagInfo.setName(tagName);
        try {
            String refTargetSha = resolveReference(repoName, tagName);
            tagInfo.setSha(refTargetSha);
            String refTargetType = getObjectInfo(repoName, refTargetSha, "t");

            if ("tag".equals(refTargetType)) {
                tagInfo.setType("annotated");
                String tagContent = getObjectInfo(repoName, refTargetSha, "p");
                String targetSha = null;
                String targetType = null;
                for (String line : tagContent.lines().collect(Collectors.toList())) {
                    if (line.startsWith("object "))
                        targetSha = line.substring(7).trim();
                    else if (line.startsWith("type "))
                        targetType = line.substring(5).trim();
                }
                tagInfo.setTargetSha(targetSha);
                tagInfo.setTargetType(targetType);
            } else {
                tagInfo.setType("lightweight");
                tagInfo.setTargetSha(refTargetSha);
                tagInfo.setTargetType(refTargetType);
            }
        } catch (Exception e) {
            System.err.println("Warning: Could not fully resolve tag details for '" + tagName + "' during tree lookup: "
                    + e.getMessage());
            // Allow proceeding if possible, handle potential nulls later
        }
        return tagInfo;
    }

    private Path getValidatedRepoPath(String repoName) {
        Path basePath = Paths.get(repositoriesBasePath).toAbsolutePath().normalize();
        Path repoPath = basePath.resolve(repoName).normalize();

        if (!repoPath.startsWith(basePath)) {
            throw new IllegalArgumentException("Invalid repository name (Path Traversal Attempt?): " + repoName);
        }
        if (!Files.isDirectory(repoPath)) {
            throw new IllegalArgumentException("Repository not found: " + repoName);
        }
        Path gitDirPath = repoPath.resolve(".mygit");
        if (!Files.isDirectory(gitDirPath) || !Files.exists(gitDirPath.resolve("HEAD"))) {
            throw new IllegalArgumentException("Not a valid mygit repository: " + repoName);
        }
        return repoPath;
    }

    private void validateShaFormat(String sha) {
        if (sha == null || !sha.matches("^[0-9a-fA-F]{1,40}$")) {
            throw new IllegalArgumentException("Invalid SHA format: " + sha);
        }
    }

    private void validateRefName(String ref) {
        if (ref == null || ref.contains("..") || ref.startsWith("/") || ref.endsWith("/") || ref.contains("//")
                || !ref.matches("^[a-zA-Z0-9/_\\-.]+$")) {
            if (!"HEAD".equals(ref) && !ref.matches("^[0-9a-fA-F]{1,40}$")) {
                throw new IllegalArgumentException("Invalid reference name format: " + ref);
            }
        }
    }

    private String buildTreeIsh(String ref, String path) {
        validateRefName(ref);
        if (path != null && (path.contains("..") || path.startsWith("/"))) {
            throw new IllegalArgumentException("Invalid path format: " + path);
        }
        return ref + (path != null && !path.isEmpty() ? ":" + path : "");
    }

    public List<String> findAllRepositoryNames() throws IOException {
        Path basePath = Paths.get(repositoriesBasePath);
        if (!Files.isDirectory(basePath)) {
            throw new IOException("Repository base path does not exist or is not a directory: " + repositoriesBasePath);
        }
        try (Stream<Path> stream = Files.list(basePath)) {
            return stream
                    .filter(Files::isDirectory)
                    .filter(p -> Files.isDirectory(p.resolve(".mygit")))
                    .map(Path::getFileName)
                    .map(Path::toString)
                    .sorted()
                    .collect(Collectors.toList());
        }
    }

    public List<FileInfo> getTreeContents(String repoName, String ref, String path, boolean recursive)
            throws IOException, InterruptedException {
        Path repoPath = getValidatedRepoPath(repoName);
        validateRefName(ref);
        // Path validation (allow null/empty for root)
        if (path != null && (path.contains("..") || path.startsWith("/"))) {
            throw new IllegalArgumentException("Invalid path format: " + path);
        }

        // --- Logic to find the target tree SHA based on ref and path ---
        String targetTreeSha;
        try {
            // 1. Resolve ref to commit/tag/tree SHA
            String resolvedSha = resolveReference(repoName, ref); // Uses rev-parse

            // 2. Get the initial tree SHA (root tree for commit/tag, or the SHA itself if
            // it's a tree)
            String initialTreeSha;
            String objectType = getObjectInfo(repoName, resolvedSha, "t"); // cat-file -t

            switch (objectType) {
                case "commit":
                    CommitInfo commitDetails = getCommitDetails(repoName, resolvedSha);
                    initialTreeSha = commitDetails.getTree();
                    break;
                case "tag":
                    // Need to dereference the tag fully to get the target commit/tree
                    TagInfo tagInfo = getTagDetails(repoName, ref); // Assuming a helper method getTagDetails exists or
                                                                    // implement here
                    if ("commit".equals(tagInfo.getTargetType())) {
                        initialTreeSha = getCommitDetails(repoName, tagInfo.getTargetSha()).getTree();
                    } else if ("tree".equals(tagInfo.getTargetType())) {
                        initialTreeSha = tagInfo.getTargetSha();
                    } else {
                        throw new IllegalArgumentException("Tag '" + ref + "' points to an object of unsupported type: "
                                + tagInfo.getTargetType());
                    }
                    break;
                case "tree":
                    initialTreeSha = resolvedSha;
                    break;
                default:
                    throw new IllegalArgumentException(
                            "Reference '" + ref + "' resolves to an object of unsupported type: " + objectType);
            }

            if (initialTreeSha == null) {
                throw new RuntimeException("Could not determine initial tree SHA for ref: " + ref);
            }
            validateShaFormat(initialTreeSha);

            // 3. Traverse path components if path is provided
            String currentTreeSha = initialTreeSha;
            if (path != null && !path.isEmpty()) {
                List<String> pathComponents = Arrays.asList(path.split("/"));
                for (String dirName : pathComponents) {
                    if (dirName.isEmpty())
                        continue; // Skip empty segments if any

                    validateShaFormat(currentTreeSha); // Validate before using
                    List<String> lsTreeCommand = List.of("mygit", "ls-tree", currentTreeSha);
                    GitCommandExecutor.CommandResult lsResult = commandExecutor.execute(repoPath.toString(),
                            lsTreeCommand);

                    if (!lsResult.isSuccess()) {
                        throw new RuntimeException("mygit ls-tree failed for intermediate tree " + currentTreeSha + ": "
                                + lsResult.stderr);
                    }

                    List<FileInfo> entries = parserService.parseLsTreeOutput(lsResult.stdout);
                    Optional<FileInfo> dirEntry = entries.stream()
                            .filter(e -> e.getName().equals(dirName) && "tree".equals(e.getType()))
                            .findFirst();

                    if (dirEntry.isEmpty()) {
                        throw new IllegalArgumentException(
                                "Directory not found in path: '" + dirName + "' within tree " + currentTreeSha);
                    }
                    currentTreeSha = dirEntry.get().getSha(); // Update to the SHA of the subdirectory tree
                }
            }
            targetTreeSha = currentTreeSha; // The final tree SHA after traversal

        } catch (IllegalArgumentException e) { // Catch validation/not found errors earlier
            System.err.println("Error resolving tree reference/path: " + e.getMessage());
            throw e; // Re-throw to be caught by controller
        } catch (Exception e) { // Catch other potential errors
            System.err.println("Unexpected error resolving tree SHA: " + e.getMessage());
            throw new RuntimeException("Failed to resolve target tree SHA for " + ref + ":" + path, e);
        }
        // --- End logic to find target tree SHA ---

        // --- Final ls-tree call using the resolved targetTreeSha ---
        validateShaFormat(targetTreeSha); // Final validation
        List<String> finalCommand = new ArrayList<>(List.of("mygit", "ls-tree"));
        if (recursive) {
            // Note: recursive flag needs careful handling with path traversal.
            // Standard git 'ls-tree -r <commit>:<path>' applies recursion *within* the
            // path.
            // This implementation lists the target tree; recursion here will list
            // *everything* under it.
            // May need adjustment depending on desired recursive behavior when path is
            // specified.
            // For now, let's assume recursion applies to the final target tree.
            if (path != null && !path.isEmpty()) {
                System.err.println(
                        "Warning: Recursive listing (-r) combined with a specific path might behave differently than standard git.");
            }
            finalCommand.add("-r");
        }
        finalCommand.add(targetTreeSha); // Use the final calculated tree SHA

        System.out.println("Executing final ls-tree command: " + String.join(" ", finalCommand)); // Debug log

        GitCommandExecutor.CommandResult result = commandExecutor.execute(repoPath.toString(), finalCommand);

        if (!result.isSuccess()) {
            // Handle case where the resolved targetTreeSha itself is somehow invalid
            // (unlikely if traversal worked)
            if (result.stderr.contains("Not a valid object name")) {
                throw new IllegalArgumentException("Resolved target tree SHA is invalid: " + targetTreeSha);
            }
            throw new RuntimeException("mygit ls-tree failed for final tree " + targetTreeSha + ": " + result.stderr);
        }

        List<FileInfo> files = parserService.parseLsTreeOutput(result.stdout);

        // If recursive and a path was given, we need to adjust the paths in FileInfo
        // This is complex as list_tree_recursive in C++ adds the prefix.
        // Let's skip path adjustment for recursive + path for now, it will show full
        // paths from repo root.

        // Sort directories first
        if (files != null) {
            Collections.sort(files, (a, b) -> {
                if (a.getType().equals(b.getType())) {
                    return a.getName().compareTo(b.getName());
                }
                return "tree".equals(a.getType()) ? -1 : 1; // trees first
            });
        }

        return files;
    }

    public String getFileContent(String repoName, String ref, String filePath)
            throws IOException, InterruptedException {
        Path repoPath = getValidatedRepoPath(repoName);
        validateRefName(ref);
        if (filePath == null || filePath.trim().isEmpty()) {
            throw new IllegalArgumentException("File path cannot be empty.");
        }

        // 1. Resolve ref to commit SHA
        String commitSha = resolveReference(repoName, ref); // Uses rev-parse

        // 2. Get root tree SHA from commit
        CommitInfo commitDetails = getCommitDetails(repoName, commitSha); // Uses cat-file -p commit
        String currentTreeSha = commitDetails.getTree();
        if (currentTreeSha == null) {
            throw new RuntimeException("Could not find root tree for commit: " + commitSha);
        }

        // 3. Traverse path components to find the final tree SHA
        List<String> pathComponents = Arrays.asList(filePath.split("/"));
        String fileName = pathComponents.get(pathComponents.size() - 1);

        // Loop through directory components (all except the last one, which is the
        // filename)
        for (int i = 0; i < pathComponents.size() - 1; i++) {
            String dirName = pathComponents.get(i);
            validateShaFormat(currentTreeSha); // Validate before using in command

            List<String> lsTreeCommand = List.of("mygit", "ls-tree", currentTreeSha);
            GitCommandExecutor.CommandResult lsResult = commandExecutor.execute(repoPath.toString(), lsTreeCommand);

            if (!lsResult.isSuccess()) {
                throw new RuntimeException("mygit ls-tree failed for tree " + currentTreeSha + ": " + lsResult.stderr);
            }

            List<FileInfo> entries = parserService.parseLsTreeOutput(lsResult.stdout);
            Optional<FileInfo> dirEntry = entries.stream()
                    .filter(e -> e.getName().equals(dirName) && "tree".equals(e.getType()))
                    .findFirst();

            if (dirEntry.isEmpty()) {
                throw new IllegalArgumentException(
                        "Directory not found in path: " + dirName + " within tree " + currentTreeSha);
            }
            currentTreeSha = dirEntry.get().getSha(); // Update to the SHA of the subdirectory tree
        }

        // 4. Find the file blob SHA in the final directory tree
        validateShaFormat(currentTreeSha); // Validate before using in command
        List<String> finalLsTreeCommand = List.of("mygit", "ls-tree", currentTreeSha);
        GitCommandExecutor.CommandResult finalLsResult = commandExecutor.execute(repoPath.toString(),
                finalLsTreeCommand);

        if (!finalLsResult.isSuccess()) {
            throw new RuntimeException(
                    "mygit ls-tree failed for final tree " + currentTreeSha + ": " + finalLsResult.stderr);
        }

        List<FileInfo> finalEntries = parserService.parseLsTreeOutput(finalLsResult.stdout);
        Optional<FileInfo> fileEntry = finalEntries.stream()
                .filter(e -> e.getName().equals(fileName) && "blob".equals(e.getType()))
                .findFirst();

        if (fileEntry.isEmpty()) {
            throw new IllegalArgumentException("File not found: " + fileName + " in tree " + currentTreeSha);
        }
        String blobSha = fileEntry.get().getSha();
        validateShaFormat(blobSha); // Validate the final blob SHA

        // 5. Get blob content using cat-file
        List<String> catFileCommand = List.of("mygit", "cat-file", "-p", blobSha);
        GitCommandExecutor.CommandResult catResult = commandExecutor.execute(repoPath.toString(), catFileCommand);

        if (!catResult.isSuccess()) {
            throw new RuntimeException("mygit cat-file -p failed for blob " + blobSha + ": " + catResult.stderr);
        }

        return parserService.parseCatFilePrettyPrintOutput(catResult.stdout);
    }

    public List<CommitInfo> getCommitHistory(String repoName, String ref, Integer limit, Integer skip)
            throws IOException, InterruptedException {
        Path repoPath = getValidatedRepoPath(repoName);
        validateRefName(ref);

        List<String> command = new ArrayList<>(List.of("mygit", "log"));
        command.add(ref);

        GitCommandExecutor.CommandResult result = commandExecutor.execute(repoPath.toString(), command);

        if (!result.isSuccess()) {
            if (result.stderr.contains("does not have any commits yet")) {
                return new ArrayList<>();
            }
            throw new RuntimeException("mygit log failed for " + repoName + "/" + ref + ": " + result.stderr);
        }

        List<CommitInfo> basicCommits = parserService.parseLogOutput(result.stdout);

        List<CommitInfo> detailedCommits = new ArrayList<>();
        for (CommitInfo basicCommit : basicCommits) {
            try {
                detailedCommits.add(getCommitDetails(repoName, basicCommit.getSha()));
            } catch (Exception e) {
                System.err.println("Warning: Failed to get full details for commit " + basicCommit.getSha()
                        + ". Using partial info. Error: " + e.getMessage());
                detailedCommits.add(basicCommit);
            }
        }

        int start = (skip != null && skip > 0) ? skip : 0;
        int end = detailedCommits.size();
        if (limit != null && limit > 0) {
            end = Math.min(start + limit, detailedCommits.size());
        }
        if (start >= detailedCommits.size()) {
            return new ArrayList<>();
        }

        return detailedCommits.subList(start, end);
    }

    public GraphData getCommitGraph(String repoName, String ref, Integer limit, Integer skip)
            throws IOException, InterruptedException {
        Path repoPath = getValidatedRepoPath(repoName);
        validateRefName(ref);

        List<String> command = List.of("mygit", "log", "--graph", ref);
        GitCommandExecutor.CommandResult result = commandExecutor.execute(repoPath.toString(), command);

        if (!result.isSuccess()) {
            if (result.stderr.contains("does not have any commits yet")) {
                return new GraphData(new ArrayList<>(), new ArrayList<>(), new ArrayList<>());
            }
            throw new RuntimeException("mygit log --graph failed for " + repoName + "/" + ref + ": " + result.stderr);
        }
        return parserService.parseGraphOutput(result.stdout);
    }

    public CommitInfo getCommitDetails(String repoName, String commitSha) throws IOException, InterruptedException {
        Path repoPath = getValidatedRepoPath(repoName);
        validateShaFormat(commitSha);

        List<String> command = List.of("mygit", "cat-file", "-p", commitSha);
        GitCommandExecutor.CommandResult result = commandExecutor.execute(repoPath.toString(), command);

        if (!result.isSuccess()) {
            if (result.stderr.contains("Not a valid object name") || result.stdout.isEmpty()) { // Check stdout too as
                                                                                                // stderr might be empty
                                                                                                // on failure
                throw new IllegalArgumentException("Commit not found: " + commitSha);
            }
            throw new RuntimeException("mygit cat-file -p " + commitSha + " failed: " + result.stderr);
        }
        String type = getObjectInfo(repoName, commitSha, "t");
        if (!"commit".equals(type)) {
            throw new IllegalArgumentException("Object is not a commit: " + commitSha + " (Type: " + type + ")");
        }

        CommitInfo commit = new CommitInfo();
        commit.setSha(commitSha);
        commit.setParents(new ArrayList<>());
        StringBuilder messageBuilder = new StringBuilder();
        boolean inMessage = false;

        for (String line : result.stdout.lines().collect(Collectors.toList())) {
            if (line.startsWith("tree ")) {
                commit.setTree(line.substring(5).trim());
                inMessage = false;
            } else if (line.startsWith("parent ")) {
                commit.getParents().add(line.substring(7).trim());
                inMessage = false;
            } else if (line.startsWith("author ")) {
                commit.setAuthor(line.substring(7).trim());
                inMessage = false;
            } else if (line.startsWith("committer ")) {
                commit.setCommitter(line.substring(10).trim());
                String committerInfo = line.substring(10).trim();
                int dateStart = committerInfo.lastIndexOf('>');
                if (dateStart != -1) {
                    commit.setDate(committerInfo.substring(dateStart + 1).trim().split("\\s+")[0]); // Extract only
                                                                                                    // timestamp part
                }
                inMessage = false;
            } else if (line.trim().isEmpty() && !inMessage) {
                inMessage = true;
            } else {
                messageBuilder.append(line).append(System.lineSeparator());
            }
        }
        commit.setMessage(messageBuilder.toString().trim());
        return commit;
    }

    public List<BranchInfo> listBranches(String repoName) throws IOException, InterruptedException {
        Path repoPath = getValidatedRepoPath(repoName);

        List<String> command = List.of("mygit", "branch");
        GitCommandExecutor.CommandResult result = commandExecutor.execute(repoPath.toString(), command);

        if (!result.isSuccess()) {
            throw new RuntimeException("mygit branch failed for " + repoName + ": " + result.stderr);
        }

        List<BranchInfo> branches = parserService.parseBranchOutput(result.stdout);

        for (BranchInfo branch : branches) {
            try {
                String sha = resolveReference(repoName, branch.getName());
                branch.setSha(sha);
            } catch (Exception e) {
                System.err.println(
                        "Warning: Could not resolve SHA for branch '" + branch.getName() + "': " + e.getMessage());
            }
        }
        return branches;
    }

    public List<TagInfo> listTags(String repoName) throws IOException, InterruptedException {
        Path repoPath = getValidatedRepoPath(repoName);

        List<String> command = List.of("mygit", "tag");
        GitCommandExecutor.CommandResult result = commandExecutor.execute(repoPath.toString(), command);

        if (!result.isSuccess()) {
            throw new RuntimeException("mygit tag failed for " + repoName + ": " + result.stderr);
        }

        List<TagInfo> tags = parserService.parseTagOutput(result.stdout);

        for (TagInfo tag : tags) {
            try {
                String refTargetSha = resolveReference(repoName, tag.getName());
                tag.setSha(refTargetSha);
                String refTargetType = getObjectInfo(repoName, refTargetSha, "t");

                if ("tag".equals(refTargetType)) {
                    tag.setType("annotated");
                    String tagContent = getObjectInfo(repoName, refTargetSha, "p"); // cat-file -p <tag_object_sha>
                    String targetSha = null;
                    String targetType = null;
                    for (String line : tagContent.lines().collect(Collectors.toList())) {
                        if (line.startsWith("object ")) {
                            targetSha = line.substring(7).trim();
                        } else if (line.startsWith("type ")) {
                            targetType = line.substring(5).trim();
                            break;
                        }
                    }
                    tag.setTargetSha(targetSha);
                    tag.setTargetType(targetType);

                } else {
                    tag.setType("lightweight");
                    tag.setTargetSha(refTargetSha);
                    tag.setTargetType(refTargetType);
                }

            } catch (Exception e) {
                System.err.println("Warning: Could not get details for tag '" + tag.getName() + "': " + e.getMessage());
                // Leave details null
            }
        }
        // Sort tags alphabetically by name
        tags.sort(Comparator.comparing(TagInfo::getName));
        return tags;
    }

    // --- Object Inspection ---
    public String resolveReference(String repoName, String ref) throws IOException, InterruptedException {
        Path repoPath = getValidatedRepoPath(repoName);
        validateRefName(ref); // Validate format before passing

        List<String> command = List.of("mygit", "rev-parse", ref);
        GitCommandExecutor.CommandResult result = commandExecutor.execute(repoPath.toString(), command);

        if (!result.isSuccess()) {
            // rev-parse returns non-zero for unknown refs
            throw new IllegalArgumentException("Reference not found: " + ref);
        }
        String sha = parserService.parseRevParseOutput(result.stdout);
        if (sha == null || sha.length() != 40) {
            throw new RuntimeException(
                    "mygit rev-parse returned unexpected output for ref '" + ref + "': " + result.stdout);
        }
        return sha;
    }

    public String getObjectInfo(String repoName, String objectShaOrPrefix, String operation)
            throws IOException, InterruptedException {
        Path repoPath = getValidatedRepoPath(repoName);
        validateShaFormat(objectShaOrPrefix); // Validate format
        if (!"t".equals(operation) && !"s".equals(operation) && !"p".equals(operation)) {
            throw new IllegalArgumentException("Invalid cat-file operation: " + operation);
        }

        List<String> command = List.of("mygit", "cat-file", "-" + operation, objectShaOrPrefix);
        GitCommandExecutor.CommandResult result = commandExecutor.execute(repoPath.toString(), command);

        if (!result.isSuccess()) {
            if (result.stderr.contains("Not a valid object name") || result.stdout.isEmpty()) {
                throw new IllegalArgumentException("Object not found: " + objectShaOrPrefix);
            }
            throw new RuntimeException(
                    "mygit cat-file -" + operation + " failed for " + objectShaOrPrefix + ": " + result.stderr);
        }

        if ("p".equals(operation)) {
            return parserService.parseCatFilePrettyPrintOutput(result.stdout);
        } else { // -t or -s
            return parserService.parseCatFileTypeOrSizeOutput(result.stdout);
        }
    }
}
