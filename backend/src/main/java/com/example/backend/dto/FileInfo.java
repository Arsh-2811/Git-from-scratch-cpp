package com.example.backend.dto;

import com.fasterxml.jackson.annotation.JsonAutoDetect;

import lombok.Data;

@Data
@JsonAutoDetect(fieldVisibility = JsonAutoDetect.Visibility.ANY)
public class FileInfo {
    private String mode;
    private String type;
    private String sha;
    private String name;
    private String path;

    public FileInfo(String mode, String type, String sha, String name, String path) {
        this.mode = mode;
        this.type = type;
        this.sha = sha;
        this.name = name;
        this.path = path;
    }

    public FileInfo() { }
}