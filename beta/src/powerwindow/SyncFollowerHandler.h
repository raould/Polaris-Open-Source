#pragma once

struct Runnable;

Runnable* makeSyncFollowerHandler(std::wstring leaderPath, std::wstring followerPath);
