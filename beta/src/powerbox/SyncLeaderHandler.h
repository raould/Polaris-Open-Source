#pragma once

struct Runnable;

Runnable* makeSyncLeaderHandler(std::wstring leaderPath, std::wstring followerPath);
