/* ============================================================
 * gui.c —— 乒乓球辅助收集程序 · 图形化界面
 *
 * 免责声明
 *   本文件的图形化界面部分（窗口、控件、消息、绘制）由 AI 生成，
 *   非本人手写，仅作学习参考，不作任何担保，使用前请自行审阅。
 *
 *   底层算法 duQuBMP / jiSuanHash / hanMingJuLi / tpruku
 *   与 kt/pingpang.h 里的数据结构，不在本声明范围内。
 *
 * 编译（必须在工程根目录）：
 *   gcc -std=c11 -Wall -Wextra -mwindows -finput-charset=GBK
 *       -fexec-charset=GBK gui.c kt/duQuBMP.c kt/jiSuanHash.c
 *       kt/hanMingJuLi.c kt/tpruku.c -o gui.exe
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <commdlg.h>
#include "kt/pingpang.h"

/* ---- 控件编号 ---- */
#define ID_XUANZE    101
#define ID_RUKU      102
#define ID_LUJING    103
#define ID_LIEBIAO   104
#define ID_ZHUANGTAI 105

/* ---- 窗口客户区 ---- */
#define CK_KUAN 900
#define CK_GAO  600

/* ---- 左侧预览区 ---- */
#define YL_X 12
#define YL_Y 12
#define YL_K 360
#define YL_G   480

/* ---- 相似度低于这个值就不进列表 ---- */
#define YUZHI 70.0f

/* ---- 列表最多显示几条 ---- */
#define ZUIDUO 10

/* ---- 图库上限，与 pingpang.h 里 Datal 的 1000 对齐 ---- */
#define TUKU_SHANGXIAN 1000

typedef struct {
    float xsd;
    char  lujing[801];
} Jieguo;

/* ---- 界面状态 ---- */
static HWND  zhuChuangkou = NULL;
static HFONT ziti = NULL;

static unsigned char *dangqianPin = NULL;   /* 当前图片像素，紧密排列、无行填充 */
static int  dangqianK = 0;
static int  dangqianG = 0;
static unsigned char dangqianHash[32];
static char dangqianLujing[801] = {0};

static unsigned char *yulanBuf = NULL;      /* 预览专用副本，行按 4 字节对齐 */

/* ============ 小工具 ============ */

static void shuaZhuangTai(const wchar_t *t)
{
    SetDlgItemTextW(zhuChuangkou, ID_ZHUANGTAI, t);
}

/* duQuBMP 的返回码翻译成人话 */
static const wchar_t *cuoWuShuoMing(int ma)
{
    switch (ma){
        case 1: return L"源文件打不开（错误1）";
        case 2: return L"文件头 14 字节读取失败（错误2）";
        case 3: return L"不是 BMP 文件（错误3）";
        case 4: return L"信息头 40 字节读取失败（错误4）";
        case 5: return L"宽或高为 0，或超过 20000（错误5）";
        case 6: return L"不是 24 位色 BMP，请另存后再试（错误6）";
        case 7: return L"malloc 分配失败（错误7）";
        case 8: return L"文件头里的像素偏移为 0（错误8）";
        case 9: return L"某一行像素读取失败（错误9）";
        default: return L"未知错误";
    }
}

/* 从路径里取文件名，'/' 和 '\' 都认。0 = 成功
   文件对话框给的是 '\'，命令行版只认 '/'，所以这里必须两种都认 */
static int zhaoWenJianMing(const char *lujing, char *shu, int chang)
{
    const char *p1 = strrchr(lujing, '/');
    const char *p2 = strrchr(lujing, '\\');
    const char *p;

    p = p1;
    if (p2 != NULL && (p == NULL || p2 > p)) p = p2;
    if (p == NULL || p[1] == '\0') return 1;
    p++;
    if (chang <= 0) return 2;
    if ((int)strlen(p) >= chang) return 2;
    strcpy(shu, p);
    return 0;
}

/* 把当前像素复制一份，行补齐到 4 字节，供 StretchDIBits 使用 */
static int zhunBeiYulan(void)
{
    int stride;
    int zong;
    int y;

    free(yulanBuf);
    yulanBuf = NULL;

    stride = (dangqianK * 3 + 3) / 4 * 4;
    zong = stride * dangqianG;
    yulanBuf = (unsigned char *)malloc((size_t)zong);
    if (yulanBuf == NULL) return 1;
    memset(yulanBuf, 0, (size_t)zong);

    for (y = 0; y < dangqianG; y++){
        memcpy(yulanBuf + (size_t)y * stride,
               dangqianPin + (size_t)y * dangqianK * 3,
               (size_t)dangqianK * 3);
    }
    return 0;
}

static int biJiao(const void *a, const void *b)
{
    const Jieguo *x = (const Jieguo *)a;
    const Jieguo *y = (const Jieguo *)b;
    if (x->xsd < y->xsd) return 1;
    if (x->xsd > y->xsd) return -1;
    return 0;
}

/* ============ 比对：扫清单，算相似度，降序放进列表 ============ */
static void duiBi(void)
{
    static Jieguo jg[TUKU_SHANGXIAN];
    FILE *fp;
    char hang[801];
    wchar_t wen[1100];
    wchar_t xian[1200];
    unsigned char h2[32];
    unsigned char *x2 = NULL;
    int k2, g2;
    int zongshu = 0;
    int mingzhong = 0;
    int shibai = 0;
    int shuliang = 0;
    int xianshi, t;
    float xsd;
    HWND liebiao;

    liebiao = GetDlgItem(zhuChuangkou, ID_LIEBIAO);
    SendMessageW(liebiao, LB_RESETCONTENT, 0, 0);

    fp = fopen("data/tukulujing.txt", "r");
    if (fp == NULL){
        shuaZhuangTai(L"打不开 data/tukulujing.txt");
        InvalidateRect(zhuChuangkou, NULL, TRUE);
        return;
    }

    while (fgets(hang, sizeof(hang), fp) != NULL){
        hang[strcspn(hang, "\r\n")] = '\0';
        if (hang[0] == '\0') continue;
        zongshu++;
        if (duQuBMP(hang, &k2, &g2, &x2) != 0){
            shibai++;
            continue;
        }
        jiSuanHash(k2, g2, x2, h2);
        xsd = (float)(256 - hanMingJuLi(dangqianHash, h2)) * 100 / 256.0f;
        free(x2);
        x2 = NULL;
        if (xsd < YUZHI) continue;
        mingzhong++;
        if (shuliang >= TUKU_SHANGXIAN) continue;   /* 装满了也继续扫，统计才准 */
        jg[shuliang].xsd = xsd;
        strncpy(jg[shuliang].lujing, hang, sizeof(jg[0].lujing) - 1);
        jg[shuliang].lujing[sizeof(jg[0].lujing) - 1] = '\0';
        shuliang++;
    }
    fclose(fp);

    qsort(jg, (size_t)shuliang, sizeof(jg[0]), biJiao);

    xianshi = (shuliang < ZUIDUO) ? shuliang : ZUIDUO;
    for (t = 0; t < xianshi; t++){
        MultiByteToWideChar(CP_ACP, 0, jg[t].lujing, -1, wen, 1100);
        swprintf(xian, 1200, L"%6.2f%%   %s", jg[t].xsd, wen);
        SendMessageW(liebiao, LB_ADDSTRING, 0, (LPARAM)xian);
    }

    if (zongshu == 0){
        shuaZhuangTai(L"图库清单是空的，先「加入图库」");
    } else if (shuliang == 0){
        swprintf(xian, 1200, L"比对 %d 张，没有相似度 ≥ 70%% 的", zongshu);
        shuaZhuangTai(xian);
    } else {
        swprintf(xian, 1200, L"比对 %d 张，命中 %d 张，最像 %.2f%%%s",
                 zongshu, mingzhong, jg[0].xsd,
                 (shibai > 0) ? L"（有读取失败）" : L"");
        shuaZhuangTai(xian);
    }
    InvalidateRect(zhuChuangkou, NULL, TRUE);
}

/* ============ 两个按钮的处理 ============ */

static void xuanZeTuPian(void)
{
    OPENFILENAMEW ofn;
    wchar_t wenLujing[801];
    char aplujing[1600];
    int k = 0, g = 0;
    int ma;
    unsigned char *p = NULL;

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = zhuChuangkou;
    ofn.lpstrFilter = L"BMP 图片\0*.bmp;*.BMP\0所有文件\0*.*\0";
    ofn.lpstrFile   = wenLujing;
    ofn.nMaxFile    = 801;
    ofn.lpstrTitle  = L"选择要收录的图片";
    ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    wenLujing[0] = L'\0';

    if (GetOpenFileNameW(&ofn) == FALSE) return;

    if (WideCharToMultiByte(CP_ACP, 0, wenLujing, -1,
                            aplujing, (int)sizeof(aplujing), NULL, NULL) == 0){
        MessageBoxW(zhuChuangkou, L"路径转换失败", L"出错", MB_OK | MB_ICONERROR);
        return;
    }

    ma = duQuBMP(aplujing, &k, &g, &p);
    if (ma != 0){
        MessageBoxW(zhuChuangkou, cuoWuShuoMing(ma), L"读取图片失败", MB_OK | MB_ICONERROR);
        return;
    }

    free(dangqianPin);
    dangqianPin = p;
    dangqianK = k;
    dangqianG = g;
    strncpy(dangqianLujing, aplujing, sizeof(dangqianLujing) - 1);
    dangqianLujing[sizeof(dangqianLujing) - 1] = '\0';
    jiSuanHash(k, g, p, dangqianHash);

    SetDlgItemTextW(zhuChuangkou, ID_LUJING, wenLujing);
    EnableWindow(GetDlgItem(zhuChuangkou, ID_RUKU), TRUE);

    if (zhunBeiYulan() != 0)
        MessageBoxW(zhuChuangkou, L"预览缓冲区分配失败", L"出错", MB_OK | MB_ICONERROR);

    duiBi();
}

static void ruKu(void)
{
    char ming[300];
    char mubiao[801];
    char linShi[801];
    wchar_t wenMing[900];
    wchar_t wenWen[1400];
    FILE *fp = NULL;
    FILE *cha = NULL;
    int ma;
    int xuan;

    if (dangqianLujing[0] == '\0'){
        MessageBoxW(zhuChuangkou, L"还没选图片", L"提示", MB_OK | MB_ICONINFORMATION);
        return;
    }

    if (zhaoWenJianMing(dangqianLujing, ming, (int)sizeof(ming)) != 0){
        MessageBoxW(zhuChuangkou, L"路径里取不到文件名", L"出错", MB_OK | MB_ICONERROR);
        return;
    }

    snprintf(mubiao, sizeof(mubiao), "data/tuku/%s", ming);

    /* ---- 同名检查必须放在这里 ----
       tpruku 内部也会查重名，但它用的是控制台菜单。
       gui.exe 是 -mwindows 程序，没有控制台，那个菜单收不到任何输入，
       scanf 会立刻返回 EOF，tpruku 就当成"用户取消"，什么都不做。
       所以在界面这一层先把重名问清楚，再给 tpruku 一条不会触发菜单的路。 */
    cha = fopen(mubiao, "rb");
    if (cha != NULL){
        fclose(cha);

        MultiByteToWideChar(CP_ACP, 0, mubiao, -1, wenMing, 900);
        swprintf(wenWen, 1400,
                 L"图库里已经有同名文件：\n\n%s\n\n"
                 L"「是」  加后缀另存（名字(1).bmp）\n"
                 L"「否」  覆盖原图\n"
                 L"「取消」不添加", wenMing);

        xuan = MessageBoxW(zhuChuangkou, wenWen, L"同名文件",
                           MB_YESNOCANCEL | MB_ICONQUESTION);

        if (xuan == IDCANCEL) return;

        if (xuan == IDYES){
            const char *dian = strrchr(ming, '.');
            int bianHao;
            int zhaoDao = 0;

            for (bianHao = 1; bianHao <= 99; bianHao++){
                if (dian == NULL || dian == ming)
                    snprintf(mubiao, sizeof(mubiao), "data/tuku/%s(%d)", ming, bianHao);
                else
                    snprintf(mubiao, sizeof(mubiao), "data/tuku/%.*s(%d)%s",
                             (int)(dian - ming), ming, bianHao, dian);

                cha = fopen(mubiao, "rb");
                if (cha == NULL){ zhaoDao = 1; break; }
                fclose(cha);
            }

            if (zhaoDao == 0){
                MessageBoxW(zhuChuangkou, L"从 (1) 试到 (99) 都被占用了",
                            L"出错", MB_OK | MB_ICONERROR);
                return;
            }
            /* 现在 mubiao 指向一个不存在的路径，tpruku 不会再问重名 */
        } else {
            /* 覆盖：先复制到临时文件，成功后才动原图。中途失败，原图不受影响 */
            snprintf(linShi, sizeof(linShi), "data/tuku/_gui_linShi.tmp");
            remove(linShi);

            ma = tpruku(dangqianLujing, linShi, ming, (int)sizeof(linShi));
            if (ma != 0){
                remove(linShi);
                swprintf(wenWen, 1400, L"复制失败（tpruku 返回 %d），原图没有动", ma);
                MessageBoxW(zhuChuangkou, wenWen, L"出错", MB_OK | MB_ICONERROR);
                return;
            }

            if (remove(mubiao) != 0){
                remove(linShi);
                MessageBoxW(zhuChuangkou,
                            L"删不掉原图（可能正被别的程序打开），本次没有覆盖",
                            L"出错", MB_OK | MB_ICONERROR);
                return;
            }

            if (rename(linShi, mubiao) != 0){
                MessageBoxW(zhuChuangkou,
                            L"新图已复制到 data/tuku/_gui_linShi.tmp，但改名失败。\n"
                            L"请手动把它改成原来的文件名。",
                            L"出错", MB_OK | MB_ICONERROR);
                return;
            }

            MultiByteToWideChar(CP_ACP, 0, mubiao, -1, wenMing, 900);
            swprintf(wenWen, 1400, L"已覆盖原图：\n\n%s", wenMing);
            MessageBoxW(zhuChuangkou, wenWen, L"成功", MB_OK | MB_ICONINFORMATION);

            EnableWindow(GetDlgItem(zhuChuangkou, ID_RUKU), FALSE);
            duiBi();
            return;      /* 覆盖这条路到此收工。路径没变，清单里本来就有这一行 */
        }
    }

    /* 到这里只剩两种情况：本来就没同名，或者选了加后缀。
       两条路 mubiao 都指向一个不存在的文件 */
    fp = fopen("data/tukulujing.txt", "a");
    if (fp == NULL){
        MessageBoxW(zhuChuangkou, L"打不开 data/tukulujing.txt", L"出错", MB_OK | MB_ICONERROR);
        return;
    }

    ma = tpruku(dangqianLujing, mubiao, ming, (int)sizeof(mubiao));
    if (ma != 0){
        fclose(fp);
        swprintf(wenWen, 1400, L"复制图片失败（tpruku 返回 %d）", ma);
        MessageBoxW(zhuChuangkou, wenWen, L"出错", MB_OK | MB_ICONERROR);
        return;
    }

    fprintf(fp, "%s\n", mubiao);
    fclose(fp);

    MultiByteToWideChar(CP_ACP, 0, mubiao, -1, wenMing, 900);
    swprintf(wenWen, 1400, L"已加入图库：\n\n%s", wenMing);
    MessageBoxW(zhuChuangkou, wenWen, L"成功", MB_OK | MB_ICONINFORMATION);

    EnableWindow(GetDlgItem(zhuChuangkou, ID_RUKU), FALSE);
    duiBi();
}

/* ============ 窗口过程 ============ */

LRESULT CALLBACK WndProc(HWND hwnd, UINT xiaoxi, WPARAM wParam, LPARAM lParam)
{
    switch (xiaoxi){

    case WM_CREATE:
    {
        HINSTANCE qi = ((LPCREATESTRUCTW)lParam)->hInstance;
        HWND c;

        zhuChuangkou = hwnd;
        ziti = CreateFontW(-16, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
                           OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           CLEARTYPE_QUALITY, DEFAULT_PITCH, L"微软雅黑");

        c = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_READONLY | ES_AUTOHSCROLL,
                384, 12, 504, 26, hwnd, (HMENU)ID_LUJING, qi, NULL);
        SendMessageW(c, WM_SETFONT, (WPARAM)ziti, TRUE);

        c = CreateWindowExW(0, L"BUTTON", L"选择图片…",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                384, 50, 130, 32, hwnd, (HMENU)ID_XUANZE, qi, NULL);
        SendMessageW(c, WM_SETFONT, (WPARAM)ziti, TRUE);

        c = CreateWindowExW(0, L"BUTTON", L"加入图库",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                524, 50, 130, 32, hwnd, (HMENU)ID_RUKU, qi, NULL);
        SendMessageW(c, WM_SETFONT, (WPARAM)ziti, TRUE);
        EnableWindow(c, FALSE);

        c = CreateWindowExW(0, L"STATIC", L"同名文件会弹窗询问",
                WS_CHILD | WS_VISIBLE,
                664, 58, 224, 20, hwnd, NULL, qi, NULL);
        SendMessageW(c, WM_SETFONT, (WPARAM)ziti, TRUE);

        c = CreateWindowExW(0, L"STATIC", L"比对结果（相似度从高到低，最多 10 条）",
                WS_CHILD | WS_VISIBLE,
                384, 92, 504, 20, hwnd, NULL, qi, NULL);
        SendMessageW(c, WM_SETFONT, (WPARAM)ziti, TRUE);

        c = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
                384, 114, 504, 378, hwnd, (HMENU)ID_LIEBIAO, qi, NULL);
        SendMessageW(c, WM_SETFONT, (WPARAM)ziti, TRUE);

        c = CreateWindowExW(0, L"STATIC", L"点「选择图片」开始",
                WS_CHILD | WS_VISIBLE,
                384, 500, 504, 22, hwnd, (HMENU)ID_ZHUANGTAI, qi, NULL);
        SendMessageW(c, WM_SETFONT, (WPARAM)ziti, TRUE);

        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)){
        case ID_XUANZE: xuanZeTuPian(); return 0;
        case ID_RUKU:   ruKu();         return 0;
        }
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc;
        RECT r;

        hdc = BeginPaint(hwnd, &ps);

        SetRect(&r, YL_X, YL_Y, YL_X + YL_K, YL_Y + YL_G);
        FillRect(hdc, &r, (HBRUSH)GetStockObject(WHITE_BRUSH));
        FrameRect(hdc, &r, (HBRUSH)GetStockObject(GRAY_BRUSH));

        if (yulanBuf != NULL && dangqianK > 0 && dangqianG > 0){
            BITMAPINFO bi;
            double sx, sy, bili;
            int dw, dh, dx, dy;

            ZeroMemory(&bi, sizeof(bi));
            bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
            bi.bmiHeader.biWidth       = dangqianK;
            bi.bmiHeader.biHeight      = dangqianG;  /* 正数 = 自下而上，和 BMP 原始顺序一致 */
            bi.bmiHeader.biPlanes      = 1;
            bi.bmiHeader.biBitCount    = 24;
            bi.bmiHeader.biCompression = BI_RGB;

            sx = (double)YL_K / (double)dangqianK;
            sy = (double)YL_G / (double)dangqianG;
            bili = (sx < sy) ? sx : sy;
            dw = (int)((double)dangqianK * bili);
            dh = (int)((double)dangqianG * bili);
            dx = YL_X + (YL_K - dw) / 2;
            dy = YL_Y + (YL_G - dh) / 2;

            SetStretchBltMode(hdc, HALFTONE);
            SetBrushOrgEx(hdc, 0, 0, NULL);
            StretchDIBits(hdc, dx, dy, dw, dh, 0, 0, dangqianK, dangqianG,
                          yulanBuf, &bi, DIB_RGB_COLORS, SRCCOPY);
        } else {
            SetBkMode(hdc, TRANSPARENT);
            DrawTextW(hdc, L"尚未选择图片", -1, &r,
                      DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        free(dangqianPin);
        free(yulanBuf);
        if (ziti != NULL) DeleteObject(ziti);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, xiaoxi, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE qi, HINSTANCE shang, LPSTR canshu, int xianshi)
{
    WNDCLASSW wc;
    MSG xiaoxi;
    RECT r;
    HWND hwnd;

    (void)shang;
    (void)canshu;
    (void)xianshi;

    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = qi;
    wc.hCursor       = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"PingPangShouLu";

    if (RegisterClassW(&wc) == 0) return 1;

    SetRect(&r, 0, 0, CK_KUAN, CK_GAO);
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);

    hwnd = CreateWindowExW(0, L"PingPangShouLu", L"乒乓球辅助收集程序",
                           WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                           r.right - r.left, r.bottom - r.top,
                           NULL, NULL, qi, NULL);
    if (hwnd == NULL) return 1;

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    while (GetMessageW(&xiaoxi, NULL, 0, 0) > 0){
        TranslateMessage(&xiaoxi);
        DispatchMessageW(&xiaoxi);
    }
    return (int)xiaoxi.wParam;
}
