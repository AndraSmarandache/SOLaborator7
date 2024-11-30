package edu.ucv.andra.so.lab7;


import java.util.ArrayList;
import java.util.List;

public class Process1 {
    public List<ColoredThread> createWhiteThreads(ResourceAccess resourceAccess) {
        List<ColoredThread> whiteThreads = new ArrayList<>();
        for (int i = 0; i < 5; i++) {
            ColoredThread whiteThread = new ColoredThread("white", resourceAccess);
            whiteThreads.add(whiteThread);
        }
        return whiteThreads;
    }
}