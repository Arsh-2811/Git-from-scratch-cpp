package com.example.backend.controller;

import java.io.IOException;
import java.util.List;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import com.example.backend.dto.BranchInfo;
import com.example.backend.service.ViewService;

@RestController
@RequestMapping("/api/repos/{repoName}/branches")
public class BranchController {
    @Autowired
    private ViewService viewService;

    /**
     * @Description: Lists all branches in the repository.
     * @HTTPMethod: GET
     * @Path: /api/repos/{repoName}/branches
     * @ReturnType: List<BranchInfo> (includes name, current status, and resolved SHA)
     * @MygitCommand: `mygit branch` (and `mygit rev-parse <branch>` for SHAs)
     */
    @GetMapping
    public ResponseEntity<List<BranchInfo>> listBranches(@PathVariable String repoName) {
        try {
            // Service validation
            List<BranchInfo> branches = viewService.listBranches(repoName);
            return ResponseEntity.ok(branches);
        } catch (IllegalArgumentException e) {
             System.err.println("Bad request for branches: " + e.getMessage());
             return ResponseEntity.status(HttpStatus.BAD_REQUEST).body(null); // Or 404
         } catch (IOException | InterruptedException e) {
            System.err.println("Error listing branches: " + e.getMessage());
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).body(null);
        }
    }
}