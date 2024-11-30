package edu.ucv.andra.so.lab7;

public class ColoredThread extends Thread {
    private String color;
    private ResourceAccess resourceAccess;

    public ColoredThread(String color, ResourceAccess resourceAccess) {
        this.color = color;
        this.resourceAccess = resourceAccess;
    }

    @Override
    public void run() {
        try {
            if ("white".equals(color)) {
                resourceAccess.useWhite();
                Thread.sleep(1000);
                resourceAccess.releaseWhite();
            } else if ("black".equals(color)) {
                resourceAccess.useBlack();
                Thread.sleep(1000);
                resourceAccess.releaseBlack();
            }
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
}