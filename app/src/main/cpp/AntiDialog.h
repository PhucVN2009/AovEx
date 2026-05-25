#pragma once
#include <cstdint>

extern bool hackAntiDialog;

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
