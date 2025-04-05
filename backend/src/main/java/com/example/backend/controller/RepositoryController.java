package com.example.backend.controller;

import java.util.List;

import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import com.example.backend.dto.FileInfo;
import com.example.backend.service.RepositoryService;

@RestController
public class RepositoryController {
    public RepositoryService repositoryService;

    public RepositoryController(RepositoryService repositoryService){
        this.repositoryService = repositoryService;
    }

    @GetMapping("/")
    public String baseMethod() {
        return "Welcome to the backend!";
    }
    
    @GetMapping("/getAllRepositories")
    public List<String> getAllRepositories(){
        return repositoryService.getRepositoriesWithMygit();
    }

    @GetMapping("/test/{repoName}")
    public ResponseEntity<List<FileInfo>> listTreeContents(
        @PathVariable String repoName,
        @RequestParam(required = false, defaultValue = "HEAD") String ref,
        @RequestParam(required = false, defaultValue = "false") boolean  recursive) {

        try {
            List<FileInfo> contents = repositoryService.lsTree(repoName, ref, recursive);
            return ResponseEntity.ok(contents);
        } catch (Exception e) {
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).body(null);
        }
    }
}