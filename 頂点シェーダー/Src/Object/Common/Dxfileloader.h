#pragma once
#include "DxLib.h"
#include <string>
#include <vector>

// ファイル全体をバイト列として読み込む
// 成功: true / 失敗: false
inline bool LoadFileToBuffer(const TCHAR* path, std::vector<char>& out)
{
    out.clear();

    int fh = FileRead_open(path);
    if (fh == -1 || fh == 0) {
        return false; // ファイルが開けない
    }

    LONGLONG size = FileRead_size(path);
    if (size < 0) {
        FileRead_close(fh);
        return false;
    }

    out.resize(static_cast<size_t>(size));
    if (size > 0) {
        FileRead_read(out.data(), static_cast<int>(size), fh);
    }

    FileRead_close(fh);
    return true;
}

// ファイル全体を文字列として読み込む
// 成功: true / 失敗: false
inline bool LoadFileToString(const TCHAR* path, std::string& out)
{
    std::vector<char> buf;
    if (!LoadFileToBuffer(path, buf)) {
        return false;
    }
    out.assign(buf.begin(), buf.end());
    return true;
}

// ---- 使用例 ----
// int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
// {
//     if (DxLib_Init() == -1) return -1;
//
//     std::string text;
//     if (LoadFileToString(_T("data/test.txt"), text)) {
//         DrawString(0, 0, text.c_str(), GetColor(255, 255, 255));
//     } else {
//         DrawString(0, 0, "読み込み失敗", GetColor(255, 0, 0));
//     }
//
//     WaitKey();
//     DxLib_End();
//     return 0;
// }