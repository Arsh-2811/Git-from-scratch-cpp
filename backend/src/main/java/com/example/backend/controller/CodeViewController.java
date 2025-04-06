package com.example.backend.controller;

import java.util.List;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpStatus;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import com.example.backend.dto.FileInfo;
import com.example.backend.service.ViewService;

@RestController
@RequestMapping("/api/repos/{repoName}")
public class CodeViewController {

    @Autowired
    private ViewService viewService;

    /**
     * @Description: Lists the contents (files/dirs) of a specific tree (commit/branch/tag).
     * @HTTPMethod: GET
     * @Path: /api/repos/{repoName}/tree
     * @Path: /api/repos/{repoName}/tree/{*path} (To view subdirectories)
     * @PathVariables: repoName, path (optional subdirectory path)
     * @QueryParameters:
     *      - ref (optional): Branch, tag, or commit SHA (defaults to HEAD).
     *      - recursive (optional, boolean): List recursively (like ls-tree -r). Defaults to false.
     * @ReturnType: List<FileInfo>
     * @MygitCommand: `mygit ls-tree [-r] <tree-ish>` (where tree-ish is resolved from ref and path)
     */
    @GetMapping(value = "/tree/{**path}", produces = MediaType.APPLICATION_JSON_VALUE)
    public ResponseEntity<List<FileInfo>> listTreeContentsWithPath(
            @PathVariable String repoName,
            @PathVariable(required = false) String path,
            @RequestParam(required = false, defaultValue = "HEAD") String ref,
            @RequestParam(required = false, defaultValue = "false") boolean recursive) {
        return listTreeContents(repoName, ref, recursive, path);
    }

    @GetMapping(value = "/tree", produces = MediaType.APPLICATION_JSON_VALUE)
    public ResponseEntity<List<FileInfo>> listTreeContentsRoot(
            @PathVariable String repoName,
            @RequestParam(required = false, defaultValue = "HEAD") String ref,
            @RequestParam(required = false, defaultValue = "false") boolean recursive) {
        return listTreeContents(repoName, ref, recursive, null); // Call helper with null path
    }

    // Helper method to avoid duplication
    private ResponseEntity<List<FileInfo>> listTreeContents(String repoName, String ref, boolean recursive, String path) {
         try {
            // Service needs to validate repoName, ref, path and resolve tree-ish
            List<FileInfo> contents = viewService.getTreeContents(repoName, ref, path, recursive);
            return ResponseEntity.ok(contents);
        } catch (IllegalArgumentException e) { // For validation errors or Not Found
             System.err.println("Bad request for tree contents: " + e.getMessage());
             return ResponseEntity.status(HttpStatus.BAD_REQUEST).body(null); // Or 404 if applicable
         } catch (Exception e) {
            System.err.println("Error getting tree contents: " + e.getMessage());
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).body(null);
        }
    }

    /**
     * @Description: Gets the raw content of a specific file (blob) at a given ref.
     * @HTTPMethod: GET
     * @Path: /api/repos/{repoName}/blob/{*filePath}
     * @PathVariables: repoName, filePath (full path within the repo)
     * @QueryParameters:
     *      - ref (optional): Branch, tag, or commit SHA (defaults to HEAD).
     * @ReturnType: String (Raw file content). Content-Type ideally set based on file type.
     * @MygitCommand: `mygit cat-file -p <resolved-blob-sha>` (requires resolving ref:filePath to blob SHA first)
     */
    @GetMapping(value = "/blob/{**filePath}", produces = MediaType.TEXT_PLAIN_VALUE) // Default to text/plain
    public ResponseEntity<String> getFileContent(
            @PathVariable String repoName,
            @PathVariable String filePath,
            @RequestParam(required = false, defaultValue = "HEAD") String ref) {
         try {
            // Service needs validation and blob SHA resolution
            String content = viewService.getFileContent(repoName, ref, filePath);
            return ResponseEntity.ok(content);
         } catch (IllegalArgumentException e) {
             System.err.println("Bad request for file content: " + e.getMessage());
             return ResponseEntity.status(HttpStatus.BAD_REQUEST).body(null); // Or 404
         } catch (Exception e) {
             System.err.println("Error getting file content: " + e.getMessage());
             return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).body(null); // Use default text/plain error message?
         }
    }
}