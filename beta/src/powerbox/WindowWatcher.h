#pragma once

class EventLoop;
class ActiveWorld;
struct Runnable;

void addWindowWatcher(EventLoop* event_loop, ActiveWorld* activeWorld);
