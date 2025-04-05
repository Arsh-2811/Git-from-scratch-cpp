package com.example.backend.service;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;

@Service
public class RepositoryService {
    @Value ("${base.url}")
    private String baseUrl;

    public List<String> getRepositoriesWithMygit(){
        File baseDir = new File(baseUrl);
        List<String> validRepos = new ArrayList<>();

        if(baseDir.exists() && baseDir.isDirectory()){
            File[] directories = baseDir.listFiles(File::isDirectory);
            if(directories != null){
                for(File dir : directories){
                    File myGitFolder = new File(dir, ".mygit");
                    if(myGitFolder.exists() && myGitFolder.isDirectory()){
                        validRepos.add(dir.getName());
                    }
                }
            }
        }
        return  validRepos;
    }
}
