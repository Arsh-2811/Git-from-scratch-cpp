package com.example.backend.controller;

import java.io.IOException;
import java.util.List;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import com.example.backend.service.ViewService;

@RestController
@RequestMapping("/api/repos")
public class RepositoryController {
    @Autowired
    private ViewService viewService;

    /**
     * @Description: Lists all available 'mygit' repository names.
     * @HTTPMethod: GET
     * @Path: /api/repos
     * @ReturnType: List<String> - A list of repository names.
     * @MygitCommand: N/A (Backend scans directory)
     */
    @GetMapping
    public ResponseEntity<List<String>> getAllRepositories() {
        try {
            List<String> repoNames = viewService.findAllRepositoryNames();
            return ResponseEntity.ok(repoNames);
        } catch (IOException e) {
            // Proper error handling recommended (e.g., using @ControllerAdvice)
            System.err.println("Error fetching repository list: " + e.getMessage());
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).body(null);
        }
    }
}