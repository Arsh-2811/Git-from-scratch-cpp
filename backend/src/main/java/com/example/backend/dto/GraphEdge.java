package com.example.backend.dto;

import com.fasterxml.jackson.annotation.JsonAutoDetect;

@JsonAutoDetect(fieldVisibility = JsonAutoDetect.Visibility.ANY)
public class GraphEdge {
    private String source;
    private String target;

    public GraphEdge(String source, String target){
        this.source = source;
        this.target = target;
    }

    public GraphEdge() {}


    public String getSource() {
        return this.source;
    }

    public void setSource(String source) {
        this.source = source;
    }

    public String getTarget() {
        return this.target;
    }

    public void setTarget(String target) {
        this.target = target;
    }
}