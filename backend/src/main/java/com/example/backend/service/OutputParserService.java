package com.example.backend.service;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.springframework.stereotype.Service;

import com.example.backend.dto.BranchInfo;
import com.example.backend.dto.CommitInfo;
import com.example.backend.dto.FileInfo;
import com.example.backend.dto.GraphData;
import com.example.backend.dto.GraphEdge;
import com.example.backend.dto.GraphNode;
import com.example.backend.dto.GraphRef;
import com.example.backend.dto.TagInfo;

@Service
public class OutputParserService {
    private static final Pattern ANSI_COLOR_PATTERN = Pattern.compile("\\x1B\\[[0-?]*[ -/]*[@-~]");

    private String stripAnsi(String text) {
        if (text == null || text.isEmpty()) {
            return text;
        }
        return ANSI_COLOR_PATTERN.matcher(text).replaceAll("");
    }

    public List<FileInfo> parseLsTreeOutput(String output) {
        List<FileInfo> files = new ArrayList<>();
        if (output == null || output.isBlank()) {
            return files;
        }
        output.lines().forEach(line -> {
            String[] parts = line.split("\\s+", 4);
            if (parts.length == 4) {
                String mode = parts[0];
                String type = parts[1];
                String sha = parts[2];
                String pathAndName = parts[3].trim();
                String name = pathAndName.substring(pathAndName.lastIndexOf('/') + 1);

                System.out.println("mode : " + mode + " , type : " + type + " , sha : " + sha
                                + " , name : " + name + ", pathAndName : " + pathAndName);

                files.add(new FileInfo(mode, type, sha, name, pathAndName));
            }
        });
        System.out.println("Files : " + files);
        return files;
    }

    public String parseCatFilePrettyPrintOutput(String output) {
        return output != null ? output : "";
    }

    public List<CommitInfo> parseLogOutput(String output) {
        List<CommitInfo> commits = new ArrayList<>();
        if (output == null || output.isBlank())
            return commits;

        String[] lines = output.split(System.lineSeparator());
        CommitInfo currentCommit = null;
        StringBuilder messageBuilder = new StringBuilder();
        boolean inMessage = false;

        for (String line : lines) {
            line = stripAnsi(line);

            if (line.startsWith("commit ")) {
                if (currentCommit != null) {
                    currentCommit.setMessage(messageBuilder.toString().trim());
                    commits.add(currentCommit);
                }

                currentCommit = new CommitInfo();
                currentCommit.setSha(line.substring(7).trim());
                currentCommit.setParents(new ArrayList<>()); // Initialize parents list
                messageBuilder = new StringBuilder();
                inMessage = false;
            } else if (currentCommit != null) {
                if (line.startsWith("Merge:")) {
                    String[] parents = line.substring(6).trim().split("\\s+");
                    currentCommit.getParents().addAll(Arrays.asList(parents));
                } else if (line.startsWith("Author: ")) {
                    currentCommit.setAuthor(line.substring(8).trim());
                    inMessage = false;
                } else if (line.startsWith("Date:   ")) {
                    currentCommit.setDate(line.substring(8).trim());
                    inMessage = false;
                } else if (line.trim().isEmpty() && !inMessage) {
                    inMessage = true;
                } else if (inMessage || line.startsWith("    ")) {
                    inMessage = true;
                    messageBuilder.append(line.startsWith("    ") ? line.substring(4) : line)
                            .append(System.lineSeparator());
                }
            }
        }

        if (currentCommit != null) {
            currentCommit.setMessage(messageBuilder.toString().trim());
            commits.add(currentCommit);
        }
        return commits;
    }

    public GraphData parseGraphOutput(String dotOutput) {
        GraphData graphData = new GraphData(new ArrayList<>(), new ArrayList<>(), new ArrayList<>());
        if (dotOutput == null || dotOutput.isBlank()) {
            return graphData;
        }

        Pattern nodePattern = Pattern.compile("\"([0-9a-fA-F]+)\"\\s*\\[label=\"(.*?)\".*?\\];");
        Pattern edgePattern = Pattern.compile("\"([0-9a-fA-F]+)\"\\s*->\\s*\"([0-9a-fA-F]+)\";");
        Pattern refNodePattern = Pattern.compile("\"(branch_|tag_|HEAD)(.*?)\"\\s*\\[label=\"(.*?)\".*?\\];");
        Pattern refEdgePattern = Pattern.compile("\"(branch_|tag_|HEAD)(.*?)\"\\s*->\\s*\"([0-9a-fA-F]+)\";");

        dotOutput.lines().forEach(line -> {
            Matcher nodeMatcher = nodePattern.matcher(line);
            if (nodeMatcher.find()) {
                String label = nodeMatcher.group(2).replace("\\n", "\n");
                graphData.getNodes().add(new GraphNode(nodeMatcher.group(1), label));
                return;
            }

            Matcher edgeMatcher = edgePattern.matcher(line);
            if (edgeMatcher.find()) {
                graphData.getEdges().add(new GraphEdge(edgeMatcher.group(1), edgeMatcher.group(2)));
                return;
            }

            Matcher refNodeMatcher = refNodePattern.matcher(line);
            if (refNodeMatcher.find()) {
                String typePrefix = refNodeMatcher.group(1); // "branch_", "tag_", "HEAD"
                String uniqueName = typePrefix + refNodeMatcher.group(2);
                String label = refNodeMatcher.group(3);
                String type = typePrefix.replace("_", ""); // "branch", "tag", "HEAD"
                graphData.getRefs().add(new GraphRef(uniqueName, label, type, null));
                return;
            }

            Matcher refEdgeMatcher = refEdgePattern.matcher(line);
            if (refEdgeMatcher.find()) {
                String typePrefix = refEdgeMatcher.group(1);
                String uniqueName = typePrefix + refEdgeMatcher.group(2);
                String targetSha = refEdgeMatcher.group(3);
                graphData.getRefs().stream()
                        .filter(ref -> ref.getName().equals(uniqueName))
                        .findFirst()
                        .ifPresent(ref -> ref.setTarget(targetSha));
                return;
            }

        });

        graphData.getRefs().removeIf(ref -> ref.getTarget() == null);

        return graphData;
    }

    public List<BranchInfo> parseBranchOutput(String output) {
        List<BranchInfo> branches = new ArrayList<>();
        if (output == null || output.isBlank()) {
            return branches;
        }
        output.lines()
            .map(this::stripAnsi) // Assuming stripAnsi is defined
            .map(String::trim)     // Trim whitespace
            .filter(line -> !line.isEmpty()) // <<<--- ADD THIS FILTER
            .forEach(line -> {
                boolean isCurrent = line.startsWith("* ");
                String name = isCurrent ? line.substring(2).trim() : line.trim();
                if (!name.isEmpty()) { // Double check name is not empty after processing
                    branches.add(new BranchInfo(name, isCurrent, null));
                }
            });
        return branches;
    }

    public List<TagInfo> parseTagOutput(String output) {
        List<TagInfo> tags = new ArrayList<>();
        if (output == null || output.isBlank()) {
            return tags;
        }
        output.lines().forEach(line -> {
            String name = stripAnsi(line.trim());
            if (!name.isEmpty()) {
                tags.add(new TagInfo(name, null, null));
            }
        });
        return tags;
    }

    public String parseRevParseOutput(String output) {
        if (output != null && !output.isBlank()) {
            return output.lines().findFirst().map(String::trim).orElse(null);
        }
        return null;
    }

    public String parseCatFileTypeOrSizeOutput(String output) {
        if (output != null && !output.isBlank()) {
            return output.lines().findFirst().map(String::trim).orElse(null);
       }
       return null;
    }
}
