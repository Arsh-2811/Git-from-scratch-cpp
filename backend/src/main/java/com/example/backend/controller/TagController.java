package com.example.backend.controller;

import java.util.List;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import com.example.backend.dto.TagInfo;
import com.example.backend.service.ViewService;

@RestController
@RequestMapping("/api/repos/{repoName}/tags")
public class TagController {

    @Autowired
    private ViewService viewService;

    /**
     * @Description: Lists all tags in the repository.
     * @HTTPMethod: GET
     * @Path: /api/repos/{repoName}/tags
     * @ReturnType: List<TagInfo> (includes name, type, resolved target SHA/Type)
     * @MygitCommand: `mygit tag` (and `mygit cat-file -t/-p <tag>` for details)
     */
    @GetMapping
    public ResponseEntity<List<TagInfo>> listTags(@PathVariable String repoName) {
        try {
            // Service validation
            List<TagInfo> tags = viewService.listTags(repoName);
            return ResponseEntity.ok(tags);
         } catch (IllegalArgumentException e) {
             System.err.println("Bad request for tags: " + e.getMessage());
             return ResponseEntity.status(HttpStatus.BAD_REQUEST).body(null); // Or 404
         } catch (Exception e) {
            System.err.println("Error listing tags: " + e.getMessage());
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).body(null);
        }
    }
}