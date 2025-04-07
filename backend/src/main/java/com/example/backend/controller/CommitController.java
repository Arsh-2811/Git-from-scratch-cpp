package com.example.backend.controller;

import java.io.IOException;
import java.util.List;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import com.example.backend.dto.CommitInfo;
import com.example.backend.dto.GraphData;
import com.example.backend.service.ViewService;

@RestController
@RequestMapping("/api/repos/{repoName}/commits")
public class CommitController {

    @Autowired
    private ViewService viewService;

    /**
     * @Description: Retrieves the commit history (optionally as a graph).
     * @HTTPMethod: GET
     * @Path: /api/repos/{repoName}/commits
     * @QueryParameters:
     *      - ref (optional): Branch/tag/commit SHA start point (defaults to HEAD).
     *      - graph (optional, boolean): Return graph data instead of linear list. Defaults to false.
     *      - limit (optional, int): Limit number of commits.
     *      - skip (optional, int): Skip N commits (for pagination).
     * @ReturnType: List<CommitInfo> OR GraphData (based on 'graph' param).
     * @MygitCommand: `mygit log [<ref>]` OR `mygit log --graph [<ref>]`
     */
    @GetMapping
    public ResponseEntity<?> getCommitLog(
            @PathVariable String repoName,
            @RequestParam(required = false, defaultValue = "HEAD") String ref,
            @RequestParam(required = false, defaultValue = "false") boolean graph,
            @RequestParam(required = false) Integer limit,
            @RequestParam(required = false) Integer skip) {
        try {
            if (graph) {
                GraphData graphData = viewService.getCommitGraph(repoName, ref, limit, skip);
                return ResponseEntity.ok(graphData);
            } else {
                List<CommitInfo> commits = viewService.getCommitHistory(repoName, ref, limit, skip);
                return ResponseEntity.ok(commits);
            }
        } catch (IllegalArgumentException e) {
            System.err.println("Bad request for commit log: " + e.getMessage());
            return ResponseEntity.status(HttpStatus.BAD_REQUEST).body(null); // Or 404
        } catch (IOException | InterruptedException e) {
            System.err.println("Error getting commit log: " + e.getMessage());
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).body(null);
        }
    }

    /**
     * @Description: Gets detailed information for a single commit.
     * @HTTPMethod: GET
     * @Path: /api/repos/{repoName}/commits/{commitSha}
     * @PathVariables: repoName, commitSha
     * @ReturnType: CommitInfo
     * @MygitCommand: `mygit cat-file -p <commitSha>`
     */
    @GetMapping("/{commitSha}")
    public ResponseEntity<CommitInfo> getCommitDetails(
            @PathVariable String repoName,
            @PathVariable String commitSha) {
        try {
            CommitInfo commitDetails = viewService.getCommitDetails(repoName, commitSha);
            return ResponseEntity.ok(commitDetails);
        } catch (IllegalArgumentException e) { // Handles not found as well via service logic
            System.err.println("Bad request for commit details: " + e.getMessage());
            return ResponseEntity.status(HttpStatus.NOT_FOUND).body(null);
        } catch (IOException | InterruptedException e) {
            System.err.println("Error getting commit details: " + e.getMessage());
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).body(null);
        }
    }
}