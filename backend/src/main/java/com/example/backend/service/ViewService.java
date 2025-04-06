package com.example.backend.service;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
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
        String treeIsh = buildTreeIsh(ref, path);

        List<String> command = new ArrayList<>(List.of("mygit", "ls-tree"));
        if (recursive) {
            command.add("-r");
        }
        command.add(treeIsh);

        GitCommandExecutor.CommandResult result = commandExecutor.execute(repoPath.toString(), command);

        if (!result.isSuccess()) {
            if (result.stderr.contains("Not a valid object name") || result.stderr.contains("not a tree")) {
                throw new IllegalArgumentException("Invalid reference or path: " + treeIsh);
            }
            throw new RuntimeException("mygit ls-tree failed for " + repoName + "/" + treeIsh + ": " + result.stderr);
        }

        return parserService.parseLsTreeOutput(result.stdout);
    }

    public String getFileContent(String repoName, String ref, String filePath)
            throws IOException, InterruptedException {
        Path repoPath = getValidatedRepoPath(repoName);
        String treeIsh = buildTreeIsh(ref, filePath);
        List<String> lsTreeCommand = List.of("mygit", "ls-tree", treeIsh);
        GitCommandExecutor.CommandResult lsResult = commandExecutor.execute(repoPath.toString(), lsTreeCommand);

        if (!lsResult.isSuccess() || lsResult.stdout.isBlank()) {
            throw new IllegalArgumentException("File not found at specified ref/path: " + repoName + "/" + treeIsh);
        }

        String blobSha = null;
        String objectType = null;
        for (String line : lsResult.stdout.lines().collect(Collectors.toList())) {
            String[] parts = line.split("\\s+", 4);
            if (parts.length == 4) {
                objectType = parts[1];
                blobSha = parts[2];
                break;
            }
        }

        if (blobSha == null || !"blob".equals(objectType)) {
            throw new IllegalArgumentException("Path does not point to a file (blob): " + repoName + "/" + treeIsh);
        }
        validateShaFormat(blobSha);

        List<String> catFileCommand = List.of("mygit", "cat-file", "-p", blobSha);
        GitCommandExecutor.CommandResult catResult = commandExecutor.execute(repoPath.toString(), catFileCommand);

        if (!catResult.isSuccess()) {
            throw new RuntimeException("mygit cat-file failed for blob " + blobSha + ": " + catResult.stderr);
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
