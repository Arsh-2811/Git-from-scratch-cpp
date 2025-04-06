package com.example.backend.dto;

import com.fasterxml.jackson.annotation.JsonAutoDetect;

@JsonAutoDetect(fieldVisibility = JsonAutoDetect.Visibility.ANY)
public class BranchInfo {
    private String name;
    private boolean isCurrent; 
    private String sha;  
    
    public BranchInfo(String name, boolean isCurrent, String sha){
        this.name = name;
        this.isCurrent = isCurrent;
        this.sha = sha;
    }

    public BranchInfo() {}


    public String getName() {
        return this.name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public boolean isIsCurrent() {
        return this.isCurrent;
    }

    public boolean getIsCurrent() {
        return this.isCurrent;
    }

    public void setIsCurrent(boolean isCurrent) {
        this.isCurrent = isCurrent;
    }

    public String getSha() {
        return this.sha;
    }

    public void setSha(String sha) {
        this.sha = sha;
    }    
}