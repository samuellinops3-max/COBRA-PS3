// Cobra Arena v2 - jogo estilo slither.io, offline, nativo para PS3 (PSL1GHT)
// Controles: analogico esquerdo (ou direcional) = direcao | X / R1 / R2 = turbo
//            START = voltar ao menu | no menu: cima/baixo = nome, esq/dir = cor, X = jogar
// Som: efeitos e musica sintetizados por codigo (nenhum arquivo de audio)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <unistd.h>
#include <malloc.h>
#include <ppu-types.h>
#include <sysutil/sysutil.h>
#include <sysutil/video.h>
#include <rsx/rsx.h>
#include <io/pad.h>
/* Se o build der erro por causa do som, troque 1 por 0 na linha abaixo */
#define USE_AUDIO 1
#if USE_AUDIO
#include <audio/audio.h>
#endif

#define CB_SIZE   (0x100000)
#define HOST_SIZE (32 * 1024 * 1024)

#define WORLD  3200.0f
#define MAXSEG 420
#define NSN    10
#define NPEL   500
#define SP     7.0f
#define PI     3.14159265f

typedef struct {
    float x[MAXSEG], y[MAXSEG];
    int len, alive, respawn, boost, grow, score, tick;
    float ang, tgt;
    u32 col;
    const char *name;
} Snake;

typedef struct {
    float x, y;
    int size, active;
    u32 col;
} Pellet;

static const u32 palette[8] = {
    0x00FF4D4D, 0x0055D6FF, 0x0066FF66, 0x00FFD23F,
    0x00C77DFF, 0x00FF8FAB, 0x0000E5C0, 0x00FF9F43
};
static const char *player_names[5] = { "PLAYER", "BRASIL", "GAMER", "PS3", "VIPER" };
static const char *bot_names[9] = { "NOOB", "COBRA", "VENOM", "MAMBA", "NINJA", "TURBO", "SLY", "KING", "ZIG" };

/* ---------- video ---------- */
static gcmContextData *ctx;
static u32 W, H;
static u32 *fb[2];
static u32 fboff[2];
static u32 *back;
static int cur = 0, first_flip = 1;
static volatile int running = 1;

static void sysutil_cb(u64 status, u64 param, void *usrdata)
{
    (void)param; (void)usrdata;
    if (status == SYSUTIL_EXIT_GAME) running = 0;
}

static void init_screen(void)
{
    void *host = memalign(1024 * 1024, HOST_SIZE);
    rsxInit(&ctx, CB_SIZE, HOST_SIZE, host);

    videoState st;
    videoGetState(VIDEO_PRIMARY, 0, &st);
    u32 resid = st.displayMode.resolution;
    if (videoGetResolutionAvailability(VIDEO_PRIMARY, VIDEO_RESOLUTION_720, VIDEO_ASPECT_AUTO, 0))
        resid = VIDEO_RESOLUTION_720;

    videoResolution res;
    videoGetResolution(resid, &res);
    W = res.width;
    H = res.height;

    videoConfiguration vc;
    memset(&vc, 0, sizeof(vc));
    vc.resolution = resid;
    vc.format = VIDEO_BUFFER_FORMAT_XRGB;
    vc.aspect = VIDEO_ASPECT_AUTO;
    vc.pitch = W * 4;
    videoConfigure(VIDEO_PRIMARY, &vc, NULL, 0);
    videoGetState(VIDEO_PRIMARY, 0, &st);

    gcmSetFlipMode(GCM_FLIP_VSYNC);

    for (int i = 0; i < 2; i++) {
        fb[i] = (u32 *)rsxMemalign(64, W * H * 4);
        gcmAddressToOffset(fb[i], &fboff[i]);
        gcmSetDisplayBuffer(i, fboff[i], W * 4, W, H);
    }
    back = (u32 *)memalign(64, W * H * 4);
    memset(back, 0, W * H * 4);
    gcmResetFlipStatus();
}

static void wait_flip(void)
{
    while (gcmGetFlipStatus() != 0) usleep(200);
    gcmResetFlipStatus();
}

static void present(void)
{
    if (!first_flip) wait_flip();
    else gcmResetFlipStatus();
    memcpy(fb[cur], back, W * H * 4);
    gcmSetFlip(ctx, cur);
    rsxFlushBuffer(ctx);
    gcmSetWaitFlip(ctx);
    cur ^= 1;
    first_flip = 0;
}

/* ---------- desenho ---------- */
static u32 shade(u32 c, int pct)
{
    u32 r = ((c >> 16) & 0xFF) * pct / 100, g = ((c >> 8) & 0xFF) * pct / 100, b = (c & 0xFF) * pct / 100;
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    return (r << 16) | (g << 8) | b;
}

static void fill_rect(int x, int y, int w, int h, u32 c)
{
    int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y;
    int x1 = x + w, y1 = y + h;
    if (x1 > (int)W) x1 = W;
    if (y1 > (int)H) y1 = H;
    for (int yy = y0; yy < y1; yy++) {
        u32 *p = back + yy * W;
        for (int xx = x0; xx < x1; xx++) p[xx] = c;
    }
}

static void circle(int cx, int cy, int r, u32 c)
{
    if (r < 1) r = 1;
    if (cx + r < 0 || cx - r >= (int)W || cy + r < 0 || cy - r >= (int)H) return;
    for (int dy = -r; dy <= r; dy++) {
        int y = cy + dy;
        if (y < 0 || y >= (int)H) continue;
        int hw = (int)sqrtf((float)(r * r - dy * dy));
        int x0 = cx - hw, x1 = cx + hw;
        if (x0 < 0) x0 = 0;
        if (x1 >= (int)W) x1 = W - 1;
        u32 *p = back + y * W;
        for (int x = x0; x <= x1; x++) p[x] = c;
    }
}

/* fonte 3x5 (A-Z, 0-9 e alguns simbolos) */
typedef struct { char c; const char *p; } Glyph;
static const Glyph glyphs[] = {
    {'0', "###" "#.#" "#.#" "#.#" "###"}, {'1', ".#." "##." ".#." ".#." "###"},
    {'2', "###" "..#" "###" "#.." "###"}, {'3', "###" "..#" "###" "..#" "###"},
    {'4', "#.#" "#.#" "###" "..#" "..#"}, {'5', "###" "#.." "###" "..#" "###"},
    {'6', "###" "#.." "###" "#.#" "###"}, {'7', "###" "..#" "..#" "..#" "..#"},
    {'8', "###" "#.#" "###" "#.#" "###"}, {'9', "###" "#.#" "###" "..#" "###"},
    {'A', ".#." "#.#" "###" "#.#" "#.#"}, {'B', "##." "#.#" "##." "#.#" "##."},
    {'C', "###" "#.." "#.." "#.." "###"}, {'D', "##." "#.#" "#.#" "#.#" "##."},
    {'E', "###" "#.." "###" "#.." "###"}, {'F', "###" "#.." "###" "#.." "#.."},
    {'G', "###" "#.." "#.#" "#.#" "###"}, {'H', "#.#" "#.#" "###" "#.#" "#.#"},
    {'I', "###" ".#." ".#." ".#." "###"}, {'J', "..#" "..#" "..#" "#.#" "###"},
    {'K', "#.#" "#.#" "##." "#.#" "#.#"}, {'L', "#.." "#.." "#.." "#.." "###"},
    {'M', "#.#" "###" "###" "#.#" "#.#"}, {'N', "###" "#.#" "#.#" "#.#" "#.#"},
    {'O', "###" "#.#" "#.#" "#.#" "###"}, {'P', "###" "#.#" "###" "#.." "#.."},
    {'Q', "###" "#.#" "#.#" "###" "..#"}, {'R', "##." "#.#" "##." "#.#" "#.#"},
    {'S', "###" "#.." "###" "..#" "###"}, {'T', "###" ".#." ".#." ".#." ".#."},
    {'U', "#.#" "#.#" "#.#" "#.#" "###"}, {'V', "#.#" "#.#" "#.#" "#.#" ".#."},
    {'W', "#.#" "#.#" "###" "###" "#.#"}, {'X', "#.#" "#.#" ".#." "#.#" "#.#"},
    {'Y', "#.#" "#.#" ".#." ".#." ".#."}, {'Z', "###" "..#" ".#." "#.." "###"},
    {'-', "..." "..." "###" "..." "..."}, {':', "..." ".#." "..." ".#." "..."},
    {'.', "..." "..." "..." "..." ".#."}, {'!', ".#." ".#." ".#." "..." ".#."},
    {'<', "..#" ".#." "#.." ".#." "..#"}, {'>', "#.." ".#." "..#" ".#." "#.."},
};
static const char *glyphtab[128];

static void init_font(void)
{
    memset(glyphtab, 0, sizeof(glyphtab));
    for (unsigned i = 0; i < sizeof(glyphs) / sizeof(glyphs[0]); i++)
        glyphtab[(int)glyphs[i].c] = glyphs[i].p;
}

static int text_width(const char *s, int sc) { return (int)strlen(s) * 4 * sc - sc; }

static void glyph_run(const char *s, int x, int y, int sc, u32 c)
{
    for (; *s; s++) {
        int ch = (unsigned char)*s;
        const char *g = ch < 128 ? glyphtab[ch] : NULL;
        if (g)
            for (int r = 0; r < 5; r++)
                for (int k = 0; k < 3; k++)
                    if (g[r * 3 + k] == '#') fill_rect(x + k * sc, y + r * sc, sc, sc, c);
        x += 4 * sc;
    }
}

/* texto com sombra preta grossa para dar para ler em qualquer fundo */
static void draw_text(const char *s, int x, int y, int sc, u32 c)
{
    int o = sc / 2 > 1 ? sc / 2 : 1;
    glyph_run(s, x + o, y + o, sc, 0x00000000);
    glyph_run(s, x - 1, y, sc, 0x00000000);
    glyph_run(s, x + 1, y - 1, sc, 0x00000000);
    glyph_run(s, x, y, sc, c);
}

static void draw_center(const char *s, int cx, int y, int sc, u32 c)
{
    draw_text(s, cx - text_width(s, sc) / 2, y, sc, c);
}

/* ---------- som (sintetizado) ---------- */
#define SR 48000.0f
#define BLKS 256

typedef struct { float f, df, vol, dvol, ph; int left, wave; } Voice;
static Voice vo[6];
static int snd_boost = 0;
#if USE_AUDIO
static u32 aport;
static audioPortConfig acfg;
static int audio_ok = 0, wblk = 0;
#endif
static long msamp = 0;
static float mph1 = 0, mph2 = 0;
static unsigned noise_seed = 12345;

static float noise(void)
{
    noise_seed = noise_seed * 1664525u + 1013904223u;
    return ((noise_seed >> 9) & 0xFFFF) / 32768.0f - 1.0f;
}

static void sfx(float f0, float f1, float vol, float dur, int wave)
{
    int n = (int)(dur * SR);
    int pick = 0;
    for (int i = 0; i < 6; i++) {
        if (vo[i].left <= 0) { pick = i; break; }
        if (vo[i].left < vo[pick].left) pick = i;
    }
    Voice *v = &vo[pick];
    v->f = f0; v->df = (f1 - f0) / n; v->vol = vol; v->dvol = -vol / n;
    v->left = n; v->wave = wave; v->ph = 0;
}

static float synth_sample(void)
{
    float out = 0;
    static const float mel[16] = {
        440.0f, 523.3f, 659.3f, 523.3f, 440.0f, 523.3f, 784.0f, 659.3f,
        392.0f, 493.9f, 587.3f, 493.9f, 392.0f, 493.9f, 659.3f, 587.3f };
    static const float bas[4] = { 110.0f, 110.0f, 98.0f, 130.8f };
    int note_len = (int)(0.17f * SR);
    int step = (int)((msamp / note_len) % 16);
    float within = (float)(msamp % note_len) / note_len;
    mph1 += mel[step] / SR; if (mph1 >= 1.0f) mph1 -= 1.0f;
    mph2 += bas[step / 4] / SR; if (mph2 >= 1.0f) mph2 -= 1.0f;
    out += (mph1 < 0.5f ? 1.0f : -1.0f) * 0.035f * (1.0f - within);
    out += (mph2 < 0.5f ? 1.0f : -1.0f) * 0.045f;
    msamp++;

    if (snd_boost) out += noise() * 0.05f;

    for (int i = 0; i < 6; i++) {
        Voice *v = &vo[i];
        if (v->left <= 0) continue;
        float s;
        if (v->wave == 0) {
            v->ph += v->f / SR; if (v->ph >= 1.0f) v->ph -= 1.0f;
            s = sinf(v->ph * 2 * PI);
        } else {
            s = noise();
        }
        out += s * v->vol;
        v->f += v->df; v->vol += v->dvol; v->left--;
    }
    if (out > 0.95f) out = 0.95f;
    if (out < -0.95f) out = -0.95f;
    return out;
}

#if USE_AUDIO
static void init_audio(void)
{
    if (audioInit() != 0) return;
    audioPortParam prm;
    memset(&prm, 0, sizeof(prm));
    prm.numChannels = AUDIO_PORT_2CH;
    prm.numBlocks = AUDIO_BLOCK_8;
    prm.attr = 0;
    prm.level = 1.0f;
    if (audioPortOpen(&prm, &aport) != 0) return;
    if (audioGetPortConfig(aport, &acfg) != 0) return;
    int nb = (int)acfg.numBlocks;
    float *base = (float *)(uintptr_t)acfg.audioDataStart;
    memset(base, 0, nb * BLKS * 2 * sizeof(float));
    if (audioPortStart(aport) != 0) return;
    volatile u32 *rp = (volatile u32 *)(uintptr_t)acfg.readIndex;
    wblk = ((int)(*rp) + 2) % nb;
    audio_ok = 1;
}

static void audio_update(void)
{
    if (!audio_ok) return;
    int nb = (int)acfg.numBlocks;
    volatile u32 *rp = (volatile u32 *)(uintptr_t)acfg.readIndex;
    int rb = (int)(*rp);
    float *base = (float *)(uintptr_t)acfg.audioDataStart;
    int guard = 0;
    while (((wblk - rb + nb) % nb) < nb - 2 && guard++ < nb) {
        float *blk = base + wblk * BLKS * 2;
        for (int s = 0; s < BLKS; s++) {
            float v = synth_sample();
            blk[s * 2] = v;
            blk[s * 2 + 1] = v;
        }
        wblk = (wblk + 1) % nb;
    }
}

static void quit_audio(void)
{
    if (!audio_ok) return;
    audioPortStop(aport);
    audioPortClose(aport);
    audioQuit();
}
#else
static void init_audio(void) {}
static void audio_update(void) {}
static void quit_audio(void) {}
#endif

/* ---------- controle ---------- */
static float pad_ax = 0, pad_ay = 0, pad_mag = 0;
static int bX = 0, bO = 0, bStart = 0, bUp = 0, bDown = 0, bLeft = 0, bRight = 0, pad_boost = 0;
static int pad_ok = 0, pad_missing = 0;
static int pX, pO, pS, pU, pD, pL, pR;
static int eX, eO, eS, nav_h, nav_v;

static void read_pad(void)
{
    padInfo info;
    padData d;
    int got = 0;
    ioPadGetInfo(&info);
    for (int p = 0; p < 7; p++) {
        if (!info.status[p]) continue;
        ioPadGetData(p, &d);
        if (d.len == 0) { got = 2; break; }   /* sem dado novo: mantem o ultimo estado */
        float ax = ((int)d.ANA_L_H - 128) / 128.0f, ay = ((int)d.ANA_L_V - 128) / 128.0f;
        bLeft = d.BTN_LEFT; bRight = d.BTN_RIGHT; bUp = d.BTN_UP; bDown = d.BTN_DOWN;
        if (bLeft) ax = -1;
        if (bRight) ax = 1;
        if (bUp) ay = -1;
        if (bDown) ay = 1;
        pad_ax = ax; pad_ay = ay;
        pad_mag = sqrtf(ax * ax + ay * ay);
        pad_boost = d.BTN_CROSS || d.BTN_R1 || d.BTN_R2;
        bX = d.BTN_CROSS; bO = d.BTN_CIRCLE; bStart = d.BTN_START;
        got = 1;
        break;
    }
    if (got) {
        pad_missing = 0;
        pad_ok = 1;
    } else if (++pad_missing > 30) {
        pad_ok = 0;
        pad_ax = pad_ay = pad_mag = 0;
        pad_boost = bX = bO = bStart = bUp = bDown = bLeft = bRight = 0;
    }

    int u = bUp || pad_ay < -0.6f, dn = bDown || pad_ay > 0.6f;
    int l = bLeft || pad_ax < -0.6f, r = bRight || pad_ax > 0.6f;
    eX = bX && !pX; eO = bO && !pO; eS = bStart && !pS;
    nav_h = (r && !pR) - (l && !pL);
    nav_v = (dn && !pD) - (u && !pU);
    pX = bX; pO = bO; pS = bStart; pU = u; pD = dn; pL = l; pR = r;
}

/* ---------- jogo ---------- */
enum { ST_MENU, ST_PLAY };
static int state = ST_MENU, skin = 0, nameidx = 0, dead_t = 0, play_t = 0, frame = 0;
static Snake sn[NSN];
static Pellet pel[NPEL];
static int pelrot = 0;
static float camx = WORLD / 2, camy = WORLD / 2, gz = 0.8f;

static float frand(void) { return (float)rand() / (float)RAND_MAX; }

static float radius_of(const Snake *s)
{
    int l = s->len > 240 ? 240 : s->len;
    return 9.0f + l * 0.035f;
}

static void add_pellet(float x, float y, int size)
{
    for (int k = 0; k < NPEL; k++) {
        int i = (pelrot + k) % NPEL;
        if (!pel[i].active) {
            pel[i].x = x; pel[i].y = y; pel[i].size = size; pel[i].active = 1;
            pel[i].col = palette[rand() % 8];
            pelrot = (i + 1) % NPEL;
            return;
        }
    }
}

static int spot_is_clear(int self, float x, float y)
{
    for (int o = 0; o < NSN; o++) {
        if (o == self || !sn[o].alive) continue;
        for (int j = 0; j < sn[o].len; j += 4) {
            float dx = sn[o].x[j] - x, dy = sn[o].y[j] - y;
            if (dx * dx + dy * dy < 300.0f * 300.0f) return 0;
        }
    }
    return 1;
}

static void spawn_snake(int i)
{
    Snake *s = &sn[i];
    float x = WORLD / 2, y = WORLD / 2;
    for (int t = 0; t < 12; t++) {
        x = 400 + frand() * (WORLD - 800);
        y = 400 + frand() * (WORLD - 800);
        if (spot_is_clear(i, x, y)) break;
    }
    s->ang = frand() * 2 * PI;
    s->tgt = s->ang;
    s->len = 10;
    for (int j = 0; j < s->len; j++) {
        s->x[j] = x - cosf(s->ang) * SP * j;
        s->y[j] = y - sinf(s->ang) * SP * j;
    }
    s->alive = 1; s->respawn = 0; s->boost = 0; s->grow = 0; s->tick = 0; s->score = 0;
    if (i == 0) {
        s->col = palette[skin];
        s->name = player_names[nameidx];
    } else {
        int c = (skin + 1 + i) % 8;
        if (c == skin) c = (c + 1) % 8;
        s->col = palette[c];
        s->name = bot_names[(i - 1) % 9];
    }
}

static void kill_snake(int i, int silent)
{
    Snake *s = &sn[i];
    if (!silent)
        for (int j = 0; j < s->len; j += 2)
            add_pellet(s->x[j] + (frand() - 0.5f) * 10, s->y[j] + (frand() - 0.5f) * 10, 2);
    s->alive = 0;
    s->respawn = 240;
    if (i == 0 && !silent) {
        dead_t = 0;
        sfx(420.0f, 50.0f, 0.30f, 0.7f, 0);
        sfx(300.0f, 300.0f, 0.20f, 0.35f, 1);
    } else if (i != 0) {
        sfx(160.0f, 60.0f, 0.10f, 0.18f, 1);
    }
}

static void bot_think(int i)
{
    Snake *s = &sn[i];
    float hx = s->x[0], hy = s->y[0];
    s->tgt += (frand() - 0.5f) * 0.25f;

    if (s->tick % 6 == 0) {
        float best = 380.0f * 380.0f;
        int bi = -1;
        for (int k = 0; k < NPEL; k++) {
            if (!pel[k].active) continue;
            float dx = pel[k].x - hx, dy = pel[k].y - hy, d = dx * dx + dy * dy;
            if (d < best) { best = d; bi = k; }
        }
        if (bi >= 0) s->tgt = atan2f(pel[bi].y - hy, pel[bi].x - hx);
    }

    if (hx < 250 || hx > WORLD - 250 || hy < 250 || hy > WORLD - 250)
        s->tgt = atan2f(WORLD / 2 - hy, WORLD / 2 - hx);

    float px = hx + cosf(s->ang) * 55, py = hy + sinf(s->ang) * 55;
    float rs = radius_of(s);
    for (int o = 0; o < NSN; o++) {
        if (o == i || !sn[o].alive) continue;
        float lim = rs + radius_of(&sn[o]) + 25;
        lim *= lim;
        for (int j = 0; j < sn[o].len; j += 2) {
            float dx = sn[o].x[j] - px, dy = sn[o].y[j] - py;
            if (dx * dx + dy * dy < lim) { s->tgt = s->ang + 1.6f; return; }
        }
    }
}

static void update_snake(int i)
{
    Snake *s = &sn[i];
    if (!s->alive) {
        if (i > 0 && --s->respawn <= 0) spawn_snake(i);
        return;
    }
    s->tick++;
    if (i == 0) {
        if (pad_mag > 0.3f) s->tgt = atan2f(pad_ay, pad_ax);
        s->boost = pad_boost;
    } else {
        bot_think(i);
        s->boost = (s->len > 30 && (s->tick / 90) % 5 == 0) ? 1 : 0;
    }

    float d = s->tgt - s->ang;
    while (d > PI) d -= 2 * PI;
    while (d < -PI) d += 2 * PI;
    if (d > 0.075f) d = 0.075f;
    if (d < -0.075f) d = -0.075f;
    s->ang += d;

    float sp = 3.0f;
    if (s->boost && s->len > 12) {
        sp = 5.4f;
        if (s->tick % 10 == 0) {
            s->len--;
            add_pellet(s->x[s->len], s->y[s->len], 1);
        }
    }
    s->x[0] += cosf(s->ang) * sp;
    s->y[0] += sinf(s->ang) * sp;
    for (int j = 1; j < s->len; j++) {
        float dx = s->x[j - 1] - s->x[j], dy = s->y[j - 1] - s->y[j];
        float dd = sqrtf(dx * dx + dy * dy);
        if (dd > SP) {
            float f = (dd - SP) / dd;
            s->x[j] += dx * f;
            s->y[j] += dy * f;
        }
    }

    if (s->x[0] < 0 || s->x[0] > WORLD || s->y[0] < 0 || s->y[0] > WORLD) { kill_snake(i, 0); return; }

    float rs = radius_of(s), er = rs + 12;
    for (int k = 0; k < NPEL; k++) {
        if (!pel[k].active) continue;
        float dx = pel[k].x - s->x[0], dy = pel[k].y - s->y[0];
        if (dx * dx + dy * dy < er * er) {
            pel[k].active = 0;
            if (i == 0) sfx(650.0f, 1100.0f, 0.16f, 0.07f, 0);
            s->grow += pel[k].size;
            s->score += pel[k].size * 5;
        }
    }
    while (s->grow >= 2 && s->len < MAXSEG) {
        s->x[s->len] = s->x[s->len - 1];
        s->y[s->len] = s->y[s->len - 1];
        s->len++;
        s->grow -= 2;
    }
    if (s->len >= MAXSEG) s->grow = 0;

    for (int o = 0; o < NSN; o++) {
        if (o == i || !sn[o].alive) continue;
        float lim = (rs + radius_of(&sn[o])) * 0.75f;
        lim *= lim;
        for (int j = 0; j < sn[o].len; j++) {
            float dx = sn[o].x[j] - s->x[0], dy = sn[o].y[j] - s->y[0];
            if (dx * dx + dy * dy < lim) { kill_snake(i, 0); return; }
        }
    }
}

static void init_game(void)
{
    memset(pel, 0, sizeof(pel));
    for (int k = 0; k < NPEL; k++) add_pellet(frand() * WORLD, frand() * WORLD, 1 + (rand() % 5 == 0));
    for (int i = 1; i < NSN; i++) spawn_snake(i);
    sn[0].alive = 0;
}

static void start_game(void)
{
    spawn_snake(0);
    state = ST_PLAY;
    play_t = 0;
    dead_t = 0;
    camx = sn[0].x[0];
    camy = sn[0].y[0];
    sfx(300.0f, 900.0f, 0.20f, 0.25f, 0);
}

static void update_game(void)
{
    frame++;
    for (int i = 0; i < NSN; i++) update_snake(i);

    int count = 0;
    for (int k = 0; k < NPEL; k++) count += pel[k].active;
    for (int n = 0; n < 2 && count < NPEL - 40; n++, count++)
        add_pellet(frand() * WORLD, frand() * WORLD, 1 + (rand() % 5 == 0));

    float tx = camx, ty = camy, tz = 0.8f;
    if (state == ST_MENU) {
        for (int i = 1; i < NSN; i++)
            if (sn[i].alive) { tx = sn[i].x[0]; ty = sn[i].y[0]; break; }
        if (nav_v) { nameidx = (nameidx + nav_v + 5) % 5; sfx(500.0f, 600.0f, 0.12f, 0.05f, 0); }
        if (nav_h) { skin = (skin + nav_h + 8) % 8; sfx(700.0f, 800.0f, 0.12f, 0.05f, 0); }
        if (eX) start_game();
    } else {
        play_t++;
        if (sn[0].alive) {
            tx = sn[0].x[0];
            ty = sn[0].y[0];
            int l = sn[0].len > 300 ? 300 : sn[0].len;
            tz = 1.0f - 0.4f * l / 300.0f;
            if (eS) { kill_snake(0, 1); state = ST_MENU; }
        } else {
    