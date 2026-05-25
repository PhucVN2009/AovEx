#pragma once
// ESP Line Box - Enemy Hero Detection
// Dump: 64BIT 1.62.1.4.cs
//
// Key offsets from dump:
//   KyriosFramework.get_actorManager()  RVA: 0x89179A8   (static)
//   CameraSystem.get_MainCamera()       RVA: 0x8D53F48   (instance)
//   CameraSystem.LateUpdate()           RVA: 0x8D53F98   (hook point)
//   Camera.WorldToScreenPoint(Vector3)  RVA: 0x94ADBF4
//
//   ActorManager.HeroActors (List)      offset: 0x20
//   List<T>._items  (array ptr)         offset: 0x10
//   List<T>._size   (count)             offset: 0x18
//   Array elements start                offset: 0x20
//   PoolObjHandle<ActorLinker> size     16 bytes
//   PoolObjHandle._handleObj (ptr)      offset: 0x08 inside handle
//
//   ActorLinker._location (VInt3)       offset: 0x16C  (divide by 1000 -> Unity units)
//   ActorLinker.monsterCamp (byte)      offset: 0x1BC
//   ActorLinker.mIsHostCtrlActor (bool) offset: 0x190
//   ActorLinker.ValueComponent (ptr)    offset: 0x28
//
//   ValueLinkerComponent.actorHp        offset: 0x40
//   ValueLinkerComponent.actorHpTotal   offset: 0x44

#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <dlfcn.h>
#include <cstdint>
#include <cstring>

// ─── Types ───────────────────────────────────────────────────────────────────

struct Vec3 {
    float x, y, z;
};

// ─── ESP State ────────────────────────────────────────────────────────────────

bool hackESP = false;

struct ESPBox {
    float x1, y1;   // bottom-left  in Unity screen pixels (0,0 = bottom-left)
    float x2, y2;   // top-right
    float hp, hpMax;
    bool  valid;
};

#define ESP_MAX_ACTORS 12
static ESPBox g_espBoxes[ESP_MAX_ACTORS];
static int    g_espBoxCnt = 0;

// Screen size (updated by glViewport hook)
static int g_sw = 1280;
static int g_sh = 720;

// CameraSystem MonoBehaviour instance (captured from LateUpdate hook)
static void* g_camSysInst = nullptr;

// ─── IL2CPP Function Pointers ─────────────────────────────────────────────────

typedef void*  (*fn_GetActorMgr)();
typedef void*  (*fn_GetMainCam)(void* camSys, void* mi);
typedef Vec3   (*fn_W2S)(void* cam, Vec3 pos, void* mi);
// IsHostPlayer(ref PoolObjHandle<ActorLinker>) RVA: 0x7DC832C
// static bool — nhận pointer đến handle (16-byte struct), trả true nếu là actor local
typedef bool   (*fn_IsHostPlayer)(void* handlePtr, void* mi);

static fn_GetActorMgr  fpGetActorMgr  = nullptr;
static fn_GetMainCam   fpGetMainCam   = nullptr;
static fn_W2S          fpW2S          = nullptr;
static fn_IsHostPlayer fpIsHostPlayer = nullptr;

// ─── Memory Helpers ───────────────────────────────────────────────────────────

template<typename T>
static inline T espRd(uintptr_t base, uintptr_t off) {
    uintptr_t addr = base + off;
    if (!addr) return T{};
    return *reinterpret_cast<T*>(addr);
}

// ─── ESP Data Update (call from CameraSystem.LateUpdate hook) ────────────────

static void ESP_Update() {
    if (!hackESP) {
        g_espBoxCnt = 0;
        return;
    }

    if (!g_camSysInst || !fpGetMainCam || !fpW2S || !fpGetActorMgr) {
        g_espBoxCnt = 0;
        return;
    }

    void* cam = fpGetMainCam(g_camSysInst, nullptr);
    if (!cam) { g_espBoxCnt = 0; return; }

    void* mgr = fpGetActorMgr();
    if (!mgr) { g_espBoxCnt = 0; return; }

    // HeroActors: List<PoolObjHandle<ActorLinker>> at offset 0x20
    uintptr_t listPtr = espRd<uintptr_t>((uintptr_t)mgr, 0x20);
    if (!listPtr) { g_espBoxCnt = 0; return; }

    // List._items (T[] array) at 0x10, List._size (count) at 0x18
    uintptr_t arrPtr   = espRd<uintptr_t>(listPtr, 0x10);
    int       heroCount = espRd<int>(listPtr, 0x18);
    if (!arrPtr || heroCount <= 0 || heroCount > 20) { g_espBoxCnt = 0; return; }

    // Find local player: IsHostPlayer as instance method (actor pointer = this),
    // then fallback to mIsHostCtrlActor field (0x190).
    // If still not found, render anyway showing all non-neutral heroes.
    uint8_t myCamp = 0xFF;
    uintptr_t myActor = 0;
    for (int i = 0; i < heroCount && i < 20; i++) {
        uintptr_t handleBase = arrPtr + 0x20 + (uintptr_t)i * 0x10;
        uintptr_t actor = espRd<uintptr_t>(handleBase, 0x08);
        if (!actor) continue;
        bool isLocal = false;
        if (fpIsHostPlayer)
            isLocal = fpIsHostPlayer((void*)actor, nullptr);  // instance method: this = actor
        if (!isLocal)
            isLocal = espRd<bool>(actor, 0x190);              // mIsHostCtrlActor fallback
        if (isLocal) {
            myCamp  = espRd<uint8_t>(actor, 0x1BC);
            myActor = actor;
            break;
        }
    }

    int cnt = 0;
    for (int i = 0; i < heroCount && i < 20 && cnt < ESP_MAX_ACTORS; i++) {
        uintptr_t handleBase = arrPtr + 0x20 + (uintptr_t)i * 0x10;
        uintptr_t actor = espRd<uintptr_t>(handleBase, 0x08);
        if (!actor) continue;
        if (actor == myActor) continue;                    // skip self by pointer
        uint8_t camp = espRd<uint8_t>(actor, 0x1BC);
        if (camp == 0) continue;                           // skip neutral/minions
        if (myCamp != 0xFF && camp == myCamp) continue;   // skip allies when camp known

        // _location: VInt3 at 0x16C  (x=0x16C, y=0x170, z=0x174)
        // VInt divide by 1000 -> Unity world units
        Vec3 feet = {
            espRd<int>(actor, 0x16C) / 1000.0f,
            espRd<int>(actor, 0x170) / 1000.0f,
            espRd<int>(actor, 0x174) / 1000.0f
        };
        Vec3 head = { feet.x, feet.y + 1.8f, feet.z };

        Vec3 fSc = fpW2S(cam, feet, nullptr);
        Vec3 hSc = fpW2S(cam, head, nullptr);

        // z > 0 means in front of camera
        if (fSc.z <= 0.01f || hSc.z <= 0.01f) continue;

        float boxH = hSc.y - fSc.y;
        if (boxH < 5.0f) {
            // Fallback: distance-scaled fixed height when 3D head projection is unreliable
            boxH = (float)g_sh / 7.0f / (fSc.z * 0.05f + 1.0f);
            if (boxH < 10.0f) continue;
        }
        float boxW = boxH * 0.5f;

        // Read HP from ValueLinkerComponent at offset 0x28
        uintptr_t valComp = espRd<uintptr_t>(actor, 0x28);
        float hp = 0.0f, hpMax = 1.0f;
        if (valComp) {
            hp    = (float)espRd<int>(valComp, 0x40);  // actorHp
            hpMax = (float)espRd<int>(valComp, 0x44);  // actorHpTotal
            if (hpMax <= 0.0f) hpMax = 1.0f;
        }

        float cx = fSc.x;
        g_espBoxes[cnt++] = {
            cx - boxW * 0.5f,  // x1
            fSc.y,             // y1  (bottom – feet in screen)
            cx + boxW * 0.5f,  // x2
            hSc.y,             // y2  (top   – head in screen)
            hp, hpMax,
            true
        };
    }
    g_espBoxCnt = cnt;
}

// ─── GL Rendering ─────────────────────────────────────────────────────────────

static GLuint g_espProg = 0;
static GLuint g_espVBO  = 0;

static void ESP_InitGL() {
    if (g_espProg) return;

    const char* vs =
        "attribute vec4 a;\n"
        "void main(){ gl_Position = a; }\n";
    const char* fs =
        "precision mediump float;\n"
        "uniform vec4 c;\n"
        "void main(){ gl_FragColor = c; }\n";

    auto compSh = [](GLenum t, const char* src) -> GLuint {
        GLuint s = glCreateShader(t);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        return s;
    };

    GLuint v = compSh(GL_VERTEX_SHADER,   vs);
    GLuint f = compSh(GL_FRAGMENT_SHADER, fs);
    g_espProg = glCreateProgram();
    glAttachShader(g_espProg, v);
    glAttachShader(g_espProg, f);
    glLinkProgram(g_espProg);
    glDeleteShader(v);
    glDeleteShader(f);
    glGenBuffers(1, &g_espVBO);
}

// Convert Unity screen pixel -> OpenGL NDC
// Unity screen: (0,0)=bottom-left  →  NDC (-1,-1)=bottom-left  ✓ same direction
static inline float ndcX(float px) { return (px / (float)g_sw) * 2.0f - 1.0f; }
static inline float ndcY(float py) { return (py / (float)g_sh) * 2.0f - 1.0f; }

static void ESP_Line(float x0, float y0, float x1, float y1) {
    float verts[8] = {
        ndcX(x0), ndcY(y0), 0.0f, 1.0f,
        ndcX(x1), ndcY(y1), 0.0f, 1.0f
    };
    glBindBuffer(GL_ARRAY_BUFFER, g_espVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
    GLint loc = glGetAttribLocation(g_espProg, "a");
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
    glDrawArrays(GL_LINES, 0, 2);
    glDisableVertexAttribArray(loc);
}

static void ESP_Rect(float x1, float y1, float x2, float y2,
                     float r, float g, float b) {
    glUseProgram(g_espProg);
    GLint col = glGetUniformLocation(g_espProg, "c");
    glUniform4f(col, r, g, b, 1.0f);
    ESP_Line(x1, y1, x2, y1);  // bottom edge
    ESP_Line(x2, y1, x2, y2);  // right  edge
    ESP_Line(x2, y2, x1, y2);  // top    edge
    ESP_Line(x1, y2, x1, y1);  // left   edge
}

// Draw all queued ESP boxes – call from eglSwapBuffers hook
static void ESP_Render() {
    if (!hackESP || g_espBoxCnt == 0) return;

    ESP_InitGL();

    // Save essential GL state
    GLint  prevProg; glGetIntegerv(GL_CURRENT_PROGRAM,      &prevProg);
    GLint  prevBuf;  glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevBuf);
    GLboolean depth; glGetBooleanv(GL_DEPTH_TEST,           &depth);
    GLboolean blend; glGetBooleanv(GL_BLEND,                &blend);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glLineWidth(2.0f);

    for (int i = 0; i < g_espBoxCnt; i++) {
        const ESPBox& b = g_espBoxes[i];
        if (!b.valid) continue;

        // Red enemy box
        ESP_Rect(b.x1, b.y1, b.x2, b.y2, 1.0f, 0.0f, 0.0f);

        // HP bar – thin vertical strip on the left of the box
        float barX2 = b.x1 - 3.0f;
        float barX1 = barX2 - 3.0f;
        float barH  = b.y2 - b.y1;
        // Background (dark gray)
        ESP_Rect(barX1, b.y1, barX2, b.y2, 0.15f, 0.15f, 0.15f);
        // Foreground (green → red based on HP %)
        float ratio  = (b.hpMax > 0.0f) ? (b.hp / b.hpMax) : 0.0f;
        if (ratio < 0.0f) ratio = 0.0f;
        if (ratio > 1.0f) ratio = 1.0f;
        if (ratio > 0.0f) {
            float gr = (ratio < 0.5f) ? (ratio * 2.0f) : 1.0f;
            float rd = (ratio > 0.5f) ? ((1.0f - ratio) * 2.0f) : 1.0f;
            ESP_Rect(barX1, b.y1,
                     barX2, b.y1 + barH * ratio,
                     rd, gr, 0.0f);
        }
    }

    // Restore GL state
    if (depth) glEnable(GL_DEPTH_TEST);
    if (blend) glEnable(GL_BLEND);
    glUseProgram(prevProg);
    glBindBuffer(GL_ARRAY_BUFFER, prevBuf);
}
