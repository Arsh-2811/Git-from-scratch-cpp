package com.example.backend.controller;

import java.util.List;

import org.springframework.beans.factory.annotation.Autowired; // Keep for extraction
import org.springframework.http.HttpStatus;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.util.AntPathMatcher;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.servlet.HandlerMapping; // Keep for extraction

import com.example.backend.dto.FileInfo; // Keep for extraction
import com.example.backend.service.ViewService;

import jakarta.servlet.http.HttpServletRequest;

@RestController
@RequestMapping("/api/repos/{repoName}")
public class CodeViewController {

    @Autowired
    private ViewService viewService;

    private static final AntPathMatcher antPathMatcher = new AntPathMatcher();

    // Tree mapping using HttpServletRequest extraction (KEEP THIS)
    @GetMapping(value = "/tree/**", produces = MediaType.APPLICATION_JSON_VALUE)
    public ResponseEntity<List<FileInfo>> listTreeContentsGeneric(
            @PathVariable String repoName,
            @RequestParam(required = false, defaultValue = "HEAD") String ref,
            @RequestParam(required = false, defaultValue = "false") boolean recursive,
            HttpServletRequest request) {

        String requestUri = (String) request.getAttribute(HandlerMapping.PATH_WITHIN_HANDLER_MAPPING_ATTRIBUTE);
        String bestMatchPattern = (String) request.getAttribute(HandlerMapping.BEST_MATCHING_PATTERN_ATTRIBUTE);
        String path = null;
        if (requestUri != null && bestMatchPattern != null) {
            path = antPathMatcher.extractPathWithinPattern(bestMatchPattern, requestUri);
             if (path != null && path.startsWith("/")) {
                 path = path.length() > 1 ? path.substring(1) : null;
             }
             if ("".equals(path)) { path = null; }
        }
        System.out.println("Extracted path for /tree/**: " + path);

        try {
            List<FileInfo> contents = viewService.getTreeContents(repoName, ref, path, recursive);
            return ResponseEntity.ok(contents);
        } catch (IllegalArgumentException e) {
            System.err.println("Bad request for tree contents: " + e.getMessage());
            return ResponseEntity.status(HttpStatus.BAD_REQUEST).body(null);
        } catch (Exception e) {
            System.err.println("Error getting tree contents: " + e.getMessage());
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).body(null);
        }
    }


    /**
     * @Description: Gets the raw content of a specific file (blob) at a given ref.
     *               Uses manual path extraction.
     * @HTTPMethod: GET
     * @Path: /api/repos/{repoName}/blob/**  <-- CHANGED MAPPING
     * @PathVariables: repoName
     * @QueryParameters:
     *      - ref (optional): Branch, tag, or commit SHA (defaults to HEAD).
     * @ReturnType: String (Raw file content).
     * @MygitCommand: `mygit cat-file -p <resolved-blob-sha>`
     */
    @GetMapping(value = "/blob/**", produces = MediaType.TEXT_PLAIN_VALUE) // <-- CHANGED MAPPING
    public ResponseEntity<String> getFileContent(
            @PathVariable String repoName,
            // @PathVariable String filePath, // Removed PathVariable
            @RequestParam(required = false, defaultValue = "HEAD") String ref,
            HttpServletRequest request) { // Inject request

        // --- Manual path extraction for Blob ---
        String requestUri = (String) request.getAttribute(HandlerMapping.PATH_WITHIN_HANDLER_MAPPING_ATTRIBUTE);
        String bestMatchPattern = (String) request.getAttribute(HandlerMapping.BEST_MATCHING_PATTERN_ATTRIBUTE);
        String filePath = null; // <-- Extract filePath manually
        if (requestUri != null && bestMatchPattern != null) {
             filePath = antPathMatcher.extractPathWithinPattern(bestMatchPattern, requestUri);
             if (filePath != null && filePath.startsWith("/")) {
                 // Remove leading slash only if it's not the only character
                 filePath = filePath.length() > 1 ? filePath.substring(1) : null;
             }
             // If extraction results in empty string, treat as null/invalid for blob
             if ("".equals(filePath)) { filePath = null; }
        }
        System.out.println("Extracted path for /blob/**: " + filePath); // Debug log

        // --- Add validation: filePath cannot be null/empty for blob access ---
         if (filePath == null || filePath.trim().isEmpty()) {
              System.err.println("Bad request for file content: File path is missing or invalid.");
              // Return 404 Not Found as the path effectively doesn't point to a blob
              return ResponseEntity.status(HttpStatus.NOT_FOUND).body(null);
         }
        // --- End validation ---

        try {
            // Pass the manually extracted, cleaned path to the service
            String content = viewService.getFileContent(repoName, ref, filePath);
            return ResponseEntity.ok(content);
        } catch (IllegalArgumentException e) { // Catch specific service errors (like file not found in git)
            System.err.println("Bad request for file content: " + e.getMessage());
            // Map service's "not found" to HTTP 404
            if (e.getMessage().contains("not found")) {
                 return ResponseEntity.status(HttpStatus.NOT_FOUND).body(null);
            }
            return ResponseEntity.status(HttpStatus.BAD_REQUEST).body(null);
        } catch (Exception e) {
            System.err.println("Error getting file content: " + e.getMessage());
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).body(null);
        }
    }
}