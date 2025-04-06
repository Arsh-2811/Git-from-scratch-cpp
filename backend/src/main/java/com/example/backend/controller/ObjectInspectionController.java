package com.example.backend.controller;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpStatus;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.CrossOrigin;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import com.example.backend.service.ViewService;

@RestController
@RequestMapping("/api/repos/{repoName}/objects")
@CrossOrigin
public class ObjectInspectionController {

    @Autowired
    private ViewService viewService;

    /**
     * @Description: Resolves a ref name (branch, tag, HEAD, SHA prefix) to its full SHA-1 hash.
     * @HTTPMethod: GET
     * @Path: /api/repos/{repoName}/objects/_resolve
     * @QueryParameters: ref (required)
     * @ReturnType: String (The full SHA-1)
     * @MygitCommand: `mygit rev-parse <ref>`
     */
    @GetMapping(value = "/_resolve", produces = MediaType.TEXT_PLAIN_VALUE)
    public ResponseEntity<String> resolveReference(
            @PathVariable String repoName,
            @RequestParam String ref) {
        try {
            // Service validation
            String sha = viewService.resolveReference(repoName, ref);
            return ResponseEntity.ok(sha);
        } catch (IllegalArgumentException e) { // Handles not found
             System.err.println("Bad request for rev-parse: " + e.getMessage());
             return ResponseEntity.status(HttpStatus.NOT_FOUND).body(null);
        } catch (Exception e) {
            System.err.println("Error resolving reference: " + e.getMessage());
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).body(null);
        }
    }

    /**
     * @Description: Gets type or size information for a repository object.
     * @HTTPMethod: GET
     * @Path: /api/repos/{repoName}/objects/{objectSha}
     * @PathVariables: repoName, objectSha (full or prefix)
     * @QueryParameters: op (required: 't' for type, 's' for size)
     * @ReturnType: String (type name or size)
     * @MygitCommand: `mygit cat-file -t <objectSha>` OR `mygit cat-file -s <objectSha>`
     */
    @GetMapping(value = "/{objectSha}", produces = MediaType.TEXT_PLAIN_VALUE)
    public ResponseEntity<String> getObjectInfo(
            @PathVariable String repoName,
            @PathVariable String objectSha,
            @RequestParam String op) {
        if (!"t".equals(op) && !"s".equals(op)) {
            return ResponseEntity.badRequest().body("Invalid operation. Use 't' (type) or 's' (size).");
        }
        try {
            // Service validation
            String result = viewService.getObjectInfo(repoName, objectSha, op);
            return ResponseEntity.ok(result);
         } catch (IllegalArgumentException e) { // Handles not found
             System.err.println("Bad request for cat-file: " + e.getMessage());
             return ResponseEntity.status(HttpStatus.NOT_FOUND).body(null);
         } catch (Exception e) {
            System.err.println("Error getting object info: " + e.getMessage());
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).body(null);
        }
    }
}