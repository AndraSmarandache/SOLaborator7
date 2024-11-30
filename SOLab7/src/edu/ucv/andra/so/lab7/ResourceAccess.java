package edu.ucv.andra.so.lab7;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

public class ResourceAccess {
    private static final Lock lock = new ReentrantLock();
    private static final Condition whiteQueue = lock.newCondition();
    private static final Condition blackQueue = lock.newCondition();

    private static final int BATHROOM_CAPACITY = 5;
    private int freeResources = BATHROOM_CAPACITY;

    private int whiteWaiting = 0;
    private int blackWaiting = 0;
    private int whiteUsing = 0;
    private int blackUsing = 0;

    private boolean isWhiteTurn = true;

    public void useWhite() throws InterruptedException {
        lock.lock();
        try {
            whiteWaiting++;
            while (blackUsing > 0 || freeResources <= 0 || !isWhiteTurn) {
                whiteQueue.await();
            }
            whiteWaiting--;
            freeResources--;
            whiteUsing++;
            System.out.println("White thread is using the resource.");
            isWhiteTurn = false;
        } finally {
            lock.unlock();
        }
    }

    public void useBlack() throws InterruptedException {
        lock.lock();
        try {
            blackWaiting++;
            while (whiteUsing > 0 || freeResources <= 0 || isWhiteTurn) {
                blackQueue.await();
            }
            blackWaiting--;
            freeResources--;
            blackUsing++;
            System.out.println("Black thread is using the resource.");
            isWhiteTurn = true;
        } finally {
            lock.unlock();
        }
    }

    public void releaseWhite() {
        lock.lock();
        try {
            whiteUsing--;
            freeResources++;
            System.out.println("White thread left the resource.");
            if (blackWaiting > 0) {
                blackQueue.signal();
                System.out.println("Signaling black thread.");
            } else if (whiteWaiting > 0) {
                whiteQueue.signal();
                System.out.println("Signaling white thread.");
            }
        } finally {
            lock.unlock();
        }
    }

    public void releaseBlack() {
        lock.lock();
        try {
            blackUsing--;
            freeResources++;
            System.out.println("Black thread left the resource.");
            if (whiteWaiting > 0) {
                whiteQueue.signal();
                System.out.println("Signaling white thread.");
            } else if (blackWaiting > 0) {
                blackQueue.signal();
                System.out.println("Signaling black thread.");
            }
        } finally {
            lock.unlock();
        }
    }
}