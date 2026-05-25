#pragma once
#include <cstdint>

extern bool hackAntiDialog;

// ── OpenMessageBoxBase ────────────────────────────────────────────────────
// HÀM GỐC của mọi dialog OLDSYS — tất cả OpenMessageBox/* đều gọi đây.
// RVA: 0x72D8DEC  Return: OLDSYS_CUIFormScript* (void*)
// Params: this, strContent, isHaveCancelBtn, confirmID, cancelID, par(*),
//         isContentLeftAlign, confirmStr, cancelStr, titleStr, autoCloseTime,
//         timeUpID, isShowCloseButton, closeID, tipsContent, tipEventID,
//         isSelectTips, strContentAlignment, bHasConfirmBtn,
//         confirmCallback, cancelCallBack
void* (*old_OpenMessageBoxBase)(
    void*, void*, bool, int32_t, int32_t, void*, bool, void*,
    void*, void*, int32_t, int32_t, bool, int32_t, void*,
    int32_t, bool, int32_t, bool, void*, void*);

void* Hook_OpenMessageBoxBase(
    void* __this, void* strContent, bool isHaveCancelBtn,
    int32_t confirmID, int32_t cancelID, void* par,
    bool isContentLeftAlign, void* confirmStr,
    void* cancelStr, void* titleStr, int32_t autoCloseTime,
    int32_t timeUpID, bool isShowCloseButton, int32_t closeID,
    void* tipsContent, int32_t tipEventID, bool isSelectTips,
    int32_t strContentAlignment, bool bHasConfirmBtn,
    void* confirmCallback, void* cancelCallBack) {
    if (hackAntiDialog) return nullptr;
    return old_OpenMessageBoxBase(
        __this, strContent, isHaveCancelBtn, confirmID, cancelID, par,
        isContentLeftAlign, confirmStr, cancelStr, titleStr, autoCloseTime,
        timeUpID, isShowCloseButton, closeID, tipsContent, tipEventID,
        isSelectTips, strContentAlignment, bHasConfirmBtn,
        confirmCallback, cancelCallBack);
}

// ── Handle_NetworkOpenMessageBoxProto ──────────────────────────────────────
// Server gửi lệnh mở dialog. RVA: 0x742EB28
void (*old_Handle_NetworkOpenMessageBoxProto)(void *__this, void *cmd, void *mi);
void Hook_Handle_NetworkOpenMessageBoxProto(void *__this, void *cmd, void *mi) {
    if (hackAntiDialog) return;
    old_Handle_NetworkOpenMessageBoxProto(__this, cmd, mi);
}

// ── IOpenMessageBox (NetworkModule) ───────────────────────────────────────
// RVA: 0x742E698
void (*old_IOpenMessageBox_Net)(void *__this, void *key, int32_t evtID, void *mi);
void Hook_IOpenMessageBox_Net(void *__this, void *key, int32_t evtID, void *mi) {
    if (hackAntiDialog) return;
    old_IOpenMessageBox_Net(__this, key, evtID, mi);
}

// ── IOpenMessageBoxCallback (NetworkModule) ───────────────────────────────
// RVA: 0x742E854
void (*old_IOpenMessageBoxCallback_Net)(void *__this, void *key, void *onConfirm, void *onCancel, void *mi);
void Hook_IOpenMessageBoxCallback_Net(void *__this, void *key, void *onConfirm, void *onCancel, void *mi) {
    if (hackAntiDialog) return;
    old_IOpenMessageBoxCallback_Net(__this, key, onConfirm, onCancel, mi);
}

// ── IOpenMessageBox (LNetwork) ────────────────────────────────────────────
// RVA: 0x6F2D02C
void (*old_IOpenMessageBox_LNet)(void *__this, void *key, int32_t evtID, void *mi);
void Hook_IOpenMessageBox_LNet(void *__this, void *key, int32_t evtID, void *mi) {
    if (hackAntiDialog) return;
    old_IOpenMessageBox_LNet(__this, key, evtID, mi);
}

// ── IOpenMessageBoxCallback (LNetwork) ───────────────────────────────────
// RVA: 0x6F29208
void (*old_IOpenMessageBoxCallback_LNet)(void *__this, void *key, void *onConfirm, void *onCancel, void *mi);
void Hook_IOpenMessageBoxCallback_LNet(void *__this, void *key, void *onConfirm, void *onCancel, void *mi) {
    if (hackAntiDialog) return;
    old_IOpenMessageBoxCallback_LNet(__this, key, onConfirm, onCancel, mi);
}

// ── checkAndShowBanGameAlert ───────────────────────────────────────────────
// RVA: 0x77ED020
void (*old_checkAndShowBanGameAlert)(void *__this, bool showWindow, void *mi);
void Hook_checkAndShowBanGameAlert(void *__this, bool showWindow, void *mi) {
    if (hackAntiDialog) return;
    old_checkAndShowBanGameAlert(__this, showWindow, mi);
}

// ── OpenPunishBanGameAlert ────────────────────────────────────────────────
// RVA: 0x72E0AEC
void (*old_OpenPunishBanGameAlert)(void *__this, int32_t leftTime, int32_t totalTime, void *mi);
void Hook_OpenPunishBanGameAlert(void *__this, int32_t leftTime, int32_t totalTime, void *mi) {
    if (hackAntiDialog) return;
    old_OpenPunishBanGameAlert(__this, leftTime, totalTime, mi);
}
