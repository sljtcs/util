#include <Windows.h>
#include <string>

namespace wins_util
{
    class Clipboard
    {
    public:
        static bool setText(const std::string& text)
        {
            // 打开剪贴板
            if(!OpenClipboard(nullptr)) return false;

            // 清空剪贴板内容
            EmptyClipboard();

            // 分配内存空间并复制字符串到内存中
            size_t size = text.size() + 1; // 包括终止符
            HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, size);
            if(!hGlobal)
            {
                CloseClipboard();
                return false;
            }

            // 将字符串复制到分配的内存
            memcpy(GlobalLock(hGlobal), text.c_str(), size);

            // 解锁内存并将剪贴板内容设置为该内存
            GlobalUnlock(hGlobal);
            SetClipboardData(CF_TEXT, hGlobal);

            // 关闭剪贴板
            CloseClipboard();

            return true;
        }

        static std::string getText()
        {
            std::string result;

            // 打开剪贴板
            if(!OpenClipboard(nullptr)) return result;

            // 获取剪贴板数据（假设为文本）
            HANDLE hData = GetClipboardData(CF_TEXT);
            if(hData == nullptr)
            {
                CloseClipboard();
                return result;
            }

            // 锁定数据并转换为字符串
            char* pszText = static_cast<char*>(GlobalLock(hData));
            if(pszText != nullptr)
            {
                result = pszText;
                GlobalUnlock(hData);
            }

            // 关闭剪贴板
            CloseClipboard();

            return result;
        }
    };
}