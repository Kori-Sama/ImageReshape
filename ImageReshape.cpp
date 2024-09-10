#include <windows.h>
#include <shobjidl.h>
#include <opencv2/opencv.hpp>
#include <string>
#include <filesystem>
#include <commctrl.h>

#pragma comment(lib, "Comctl32.lib")

const wchar_t CLASS_NAME[] = L"ImageCropperWindow";
HWND hProgressBar;

void UpdateProgressBar(int progress) {
    SendMessage(hProgressBar, PBM_SETPOS, progress, 0);
}


cv::Mat CropImage(const cv::Mat& image, int targetWidth, int targetHeight) {
    int width = image.cols;
    int height = image.rows;
    float aspectRatio = static_cast<float>(targetWidth) / targetHeight;

    int newWidth = width;
    int newHeight = static_cast<int>(width / aspectRatio);

    if (newHeight > height) {
        newHeight = height;
        newWidth = static_cast<int>(height * aspectRatio);
    }

    int x = (width - newWidth) / 2;
    int y = (height - newHeight) / 2;

    cv::Rect cropRegion(x, y, newWidth, newHeight);
    return image(cropRegion);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        hProgressBar = CreateWindowExW(0, PROGRESS_CLASS, NULL,
            WS_CHILD | WS_VISIBLE, 50, 100, 300, 30, hwnd, NULL, NULL, NULL);
        SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
        return 0;
    }
    case WM_DROPFILES: {
        HDROP hDrop = (HDROP)wParam;
        UINT fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);
        int progress = 0;

        for (UINT i = 0; i < fileCount; i++) {
            wchar_t filePath[MAX_PATH];
            DragQueryFileW(hDrop, i, filePath, MAX_PATH);

            std::filesystem::path path(filePath);
            if (std::filesystem::is_directory(path)) {
                size_t totalFiles = std::distance(std::filesystem::directory_iterator(path), std::filesystem::directory_iterator{});
                size_t processedFiles = 0;

                for (const auto& entry : std::filesystem::directory_iterator(path)) {
                    if (entry.is_regular_file()) {
                        std::wstring fileName = entry.path().wstring();
                        cv::Mat image = cv::imread(cv::String(fileName.begin(), fileName.end()));

                        if (!image.empty()) {
                            cv::Mat croppedImage = CropImage(image, 1350, 1920);

                            cv::imwrite(cv::String(fileName.begin(), fileName.end()), croppedImage);
                        }
                        processedFiles++;
                        progress = static_cast<int>((processedFiles * 100) / totalFiles);
                        UpdateProgressBar(progress);
                    }
                }
            }
        }
        DragFinish(hDrop);
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        const wchar_t text[] = L"将文件夹拖放到此处";
        TextOutW(hdc, 70, 70, text, wcslen(text));

        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
}
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nShowCmd) {
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&icex);

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        0, CLASS_NAME, L"Image Cropper",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        400, 300,  
        NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nShowCmd);

    DragAcceptFiles(hwnd, TRUE);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;}
