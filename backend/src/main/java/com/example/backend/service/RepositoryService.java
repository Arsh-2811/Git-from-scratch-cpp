package com.example.backend.service;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;

import com.example.backend.dto.FileInfo;

@Service
public class RepositoryService {
    @Value ("${base.url}")
    private String baseUrl;

    @Autowired
    private GitCommandExecutor commandExecutor;

    @Autowired
    private OutputParserService parserService;

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

    private Path getValidatedRepoPath(String repoName) {
        Path basePath = Paths.get(baseUrl).toAbsolutePath().normalize();
        Path repoPath = basePath.resolve(repoName).normalize();

        if (!repoPath.startsWith(basePath)) {
            throw new IllegalArgumentException("Invalid repository name: " + repoName);
        }
        if (!Files.isDirectory(repoPath)) {
            throw new IllegalArgumentException("Repository not found: " + repoName);
        }
        Path gitDirPath = repoPath.resolve(".mygit");
        if (!Files.isDirectory(gitDirPath)) {
             throw new IllegalArgumentException("Not a valid mygit repository: " + repoName);
        }
        return repoPath;
    }

    public List<FileInfo> lsTree(String repoName, String treeIsh, boolean recursive) throws IOException, InterruptedException {
        Path repoPath = getValidatedRepoPath(repoName);

        List<String> command = new ArrayList<>(List.of("mygit", "ls-tree"));
        if (recursive) {
            command.add("-r");
        }
        command.add(treeIsh);

        GitCommandExecutor.CommandResult result = commandExecutor.execute(repoPath.toString(), command);

        if(!result.isSuccess()) throw new RuntimeException("mygit ls-tree failed: " + result.stderr);

        return parserService.parseLsTreeOutput(result.stdout);
    }
}
