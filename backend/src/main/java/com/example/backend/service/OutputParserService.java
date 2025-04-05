package com.example.backend.service;

import java.util.ArrayList;
import java.util.List;
import java.util.regex.Pattern;

import org.springframework.stereotype.Service;

import com.example.backend.dto.FileInfo;

@Service
public class OutputParserService {
    private static final Pattern ANSI_COLOR_PATTERN = Pattern.compile("\\x1B\\[[0-?]*[ -/]*[@-~]");

    private String stripAnsi(String text){
        if(text == null || text.isEmpty()) {
            return text;
        }
        return ANSI_COLOR_PATTERN.matcher(text).replaceAll("");
    }

    public List<FileInfo> parseLsTreeOutput(String output){
        List<FileInfo> files = new ArrayList<>();
        if(output == null || output.isBlank()) {
            return files;
        }
        output.lines().forEach(line -> {
            String[] parts = line.split("\\s+", 4);
            if(parts.length == 4) {
                String mode = parts[0];
                String type = parts[1];
                String sha = parts[2];
                String pathAndName = parts[3].trim();
                String name = pathAndName.substring(pathAndName.lastIndexOf('/') + 1);
                files.add(new FileInfo(mode, type, sha, name, pathAndName));
            }
        });
        return files;
    }
}
