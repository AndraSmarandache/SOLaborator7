package edu.ucv.andra.so.lab7;

import java.util.ArrayList;
import java.util.List;

public class Process2 {
    public List<ColoredThread> createBlackThreads(ResourceAccess resourceAccess) {
        List<ColoredThread> blackThreads = new ArrayList<>();
        for (int i = 0; i < 5; i++) {
            ColoredThread blackThread = new ColoredThread("black", resourceAccess);
            blackThreads.add(blackThread);
        }
        return blackThreads;
    }
}