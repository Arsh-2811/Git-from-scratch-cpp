package com.example.backend.dto;

import java.util.List;

import com.fasterxml.jackson.annotation.JsonAutoDetect;

@JsonAutoDetect(fieldVisibility = JsonAutoDetect.Visibility.ANY)
public class GraphData {
    private List<GraphNode> nodes;
    private List<GraphEdge> edges;
    private List<GraphRef> refs;

    public GraphData(List<GraphNode> nodes, List<GraphEdge> edges, List<GraphRef> refs){
        this.nodes = nodes;
        this.edges = edges;
        this.refs = refs;
    }

    public GraphData() {}

    public List<GraphNode> getNodes() {
        return this.nodes;
    }

    public void setNodes(List<GraphNode> nodes) {
        this.nodes = nodes;
    }

    public List<GraphEdge> getEdges() {
        return this.edges;
    }

    public void setEdges(List<GraphEdge> edges) {
        this.edges = edges;
    }

    public List<GraphRef> getRefs() {
        return this.refs;
    }

    public void setRefs(List<GraphRef> refs) {
        this.refs = refs;
    }
}
