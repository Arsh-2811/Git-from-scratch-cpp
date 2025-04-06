package com.example.backend.dto;

import java.util.List;

import com.fasterxml.jackson.annotation.JsonAutoDetect;

@JsonAutoDetect(fieldVisibility = JsonAutoDetect.Visibility.ANY)
public class CommitInfo {
    private String sha;
    private String tree;
    private List<String> parents;
    private String author;
    private String committer;
    private String date;
    private String message;

    public CommitInfo(String sha, String tree, List<String> parents, String author, String committer, String date, String message){
        this.sha = sha;
        this.tree = tree;
        this.parents = parents;
        this.author = author;
        this.committer = committer;
        this.date = date;
        this.message = message;
    }

    public CommitInfo(){}


    public String getSha() {
        return this.sha;
    }

    public void setSha(String sha) {
        this.sha = sha;
    }

    public String getTree() {
        return this.tree;
    }

    public void setTree(String tree) {
        this.tree = tree;
    }

    public List<String> getParents() {
        return this.parents;
    }

    public void setParents(List<String> parents) {
        this.parents = parents;
    }

    public String getAuthor() {
        return this.author;
    }

    public void setAuthor(String author) {
        this.author = author;
    }

    public String getCommitter() {
        return this.committer;
    }

    public void setCommitter(String committer) {
        this.committer = committer;
    }

    public String getDate() {
        return this.date;
    }

    public void setDate(String date) {
        this.date = date;
    }

    public String getMessage() {
        return this.message;
    }

    public void setMessage(String message) {
        this.message = message;
    }
}