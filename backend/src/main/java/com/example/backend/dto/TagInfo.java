package com.example.backend.dto;

import com.fasterxml.jackson.annotation.JsonAutoDetect;

@JsonAutoDetect(fieldVisibility = JsonAutoDetect.Visibility.ANY)
public class TagInfo {
    private String name;
    private String sha;
    private String type;
    private String targetSha;
    private String targetType;

    public TagInfo(String name, String sha, String type, String targetSha, String targetType){
        this.name = name;
        this.sha = sha;
        this.type = type;
        this.targetSha = targetSha;
        this.targetType = targetType;
    }

    public TagInfo(String name, String sha, String type){
        this.name = name;
        this.sha = sha;
        this.type = type;
    }

    public TagInfo() {}


    public String getName() {
        return this.name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getSha() {
        return this.sha;
    }

    public void setSha(String sha) {
        this.sha = sha;
    }

    public String getType() {
        return this.type;
    }

    public void setType(String type) {
        this.type = type;
    }

    public String getTargetSha() {
        return this.targetSha;
    }

    public void setTargetSha(String targetSha) {
        this.targetSha = targetSha;
    }

    public String getTargetType() {
        return this.targetType;
    }

    public void setTargetType(String targetType) {
        this.targetType = targetType;
    }
}