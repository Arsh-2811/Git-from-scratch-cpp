package com.example.backend.controller;

import java.util.List;

import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RestController;

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
}
