package com.example.backend.service;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;
import java.util.regex.Pattern;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Component;

@Component
public class GitCommandExecutor {
    private static final Logger log = LoggerFactory.getLogger(GitCommandExecutor.class);
    private static final long DEFAULT_TIMEOUT_SECONDS = 30;

    private static final Pattern ARG_SANITIZE_PATTERN = Pattern.compile("[;&|`'\"$!*?<>\\\\]");

    public static class CommandResult {
        public final int exitCode;
        public final String stdout;
        public final String stderr;

        public CommandResult(int exitCode, String stdout, String stderr) {
            this.exitCode = exitCode;
            this.stdout = stdout;
            this.stderr = stderr;
        }

        public boolean isSuccess() {
            return exitCode == 0;
        }
    }

    public CommandResult execute(String repoPath, List<String> commandAndArgs) throws IOException, InterruptedException {
        if(commandAndArgs == null || commandAndArgs.isEmpty() || !"mygit".equals(commandAndArgs.get(0))){
            throw new IllegalArgumentException("Invalid command structure. Must start with 'mygit'.");
        }

        List<String> sanitizedArgs = new ArrayList<>();
        sanitizedArgs.add(commandAndArgs.get(0));
        for (int i = 1; i < commandAndArgs.size(); i++) {
            String arg = commandAndArgs.get(i);
            if (arg == null) continue;
             if (ARG_SANITIZE_PATTERN.matcher(arg).find()) {
                 log.warn("Potential dangerous characters found in argument, skipping execution for safety: '{}'", arg);
                 return new CommandResult(1, "", "Error: Invalid characters in command arguments.");
             }
            sanitizedArgs.add(arg);
        }

        ProcessBuilder pb = new ProcessBuilder(sanitizedArgs);
        pb.directory(new File(repoPath));

        log.debug("Executing command in [{}: {}]", repoPath, String.join(" ", sanitizedArgs));
        
        Process process = pb.start();
        
        StringBuilder stdoutBuilder = new StringBuilder();
        StringBuilder stderrBuilder = new StringBuilder();

        Thread stdoutReader = new Thread(() -> {
            try (BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream()))) {
                String line;
                while((line = reader.readLine()) != null){
                    stdoutBuilder.append(line).append(System.lineSeparator());
                }
            } catch (IOException e){
                log.error("Error reading stdout: {}", e.getMessage());
            }
        });

        Thread stderrReader = new Thread(() -> {
            try (BufferedReader reader = new BufferedReader(new InputStreamReader(process.getErrorStream()))) {
                String line;
                while ((line = reader.readLine()) != null) {
                    stderrBuilder.append(line).append(System.lineSeparator());
                }
            } catch (IOException e) {
                log.error("Error reading stderr: {}", e.getMessage());
            }
        });

        stdoutReader.start();
        stderrReader.start();

        boolean finished = process.waitFor(DEFAULT_TIMEOUT_SECONDS, TimeUnit.SECONDS);

        stdoutReader.join(1000);
        stderrReader.join(1000);

        System.out.println("Stdout : " + stdoutBuilder.toString());
        System.out.println("Stderr : " + stderrBuilder.toString());

        if(!finished){
            process.destroyForcibly();
            log.error("Command timed out: {}", String.join(" ", sanitizedArgs));
            return new CommandResult(-1, stdoutBuilder.toString(), stderrBuilder.toString() + "\nError: Command timed out.");
        }

        int exitCode = process.exitValue();
        String stdout = stdoutBuilder.toString();
        String stderr = stderrBuilder.toString();

        log.debug("Command finished with exit code {}. Stdout length: {}, Stderr length: {}", exitCode, stdout.length(), stderr.length());

        if(exitCode != 0 && !stderr.isEmpty()){
            log.warn("Command stderr: {}", stderr.trim());
        }

        return new CommandResult(exitCode, stdout, stderr);
    }
}
