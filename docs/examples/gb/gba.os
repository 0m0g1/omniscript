// docs/examples/gb/gba.os
/// Game Boy Advance core, stage 1: ARM7TDMI (ARM + THUMB), memory bus, interrupts, scanline timing,
/// a high-level BIOS subset (no BIOS file needed) and the LCD for BG modes 0, 1 (text layers), 3, 4 and 5.
/// Not done yet: sprites, rotation/scaling BGs, windows, blending, DMA, timers, sound, save memory.
/// Same style as gb.os: fixed global storage, scalar fields in `GameBoyAdvance`, nothing heap-allocated.

const M32: i64 = 4294967295;
const B31: i64 = 2147483648;
const B32: i64 = 4294967296;

let GBIOS: u8[16384];
let GEWRAM: u8[262144];
let GIWRAM: u8[32768];
let GIO: u8[1024];
let GPAL: u8[1024];
let GVRAM: u8[98304];
let GOAM: u8[1024];
let GROM: u8[33554432];
let GSAVE: u8[131072];
let GTMP: u8[262144];            // scratch for BIOS decompression
let GFB: u8[153600];             // 240 x 160 RGBA
let LB: int[240];                // one scanline, BGR555

let R: i64[16];                  // visible registers
let BR13: i64[6];                // banked r13 / r14 / SPSR per bank: 0 usr+sys, 1 fiq, 2 irq, 3 svc, 4 abt, 5 und
let BR14: i64[6];
let BSPSR: i64[6];
let USRHI: i64[5];               // user r8..r12 while FIQ is active
let FIQHI: i64[5];
let W16: int[16];                // access cost by address region (16-bit)
let W32: int[16];                // access cost by address region (32-bit)

function s32(x: i64) => i64 {
    let r: i64 = x;
    if (x >= B31) { r = x - B32; }
    return r;
}

function ror32(x: i64, n: int) => i64 {
    let k: int = n & 31;
    let r: i64 = x;
    if (k != 0) { r = ((x >> k) | (x << (32 - k))) & M32; }
    return r;
}

function bankOf(mode: int) => int {
    let b: int = 0;
    if (mode == 0x11) { b = 1; }
    if (mode == 0x12) { b = 2; }
    if (mode == 0x13) { b = 3; }
    if (mode == 0x17) { b = 4; }
    if (mode == 0x1B) { b = 5; }
    return b;
}

function mk32(b0: int, b1: int, b2: int, b3: int) => i64 {
    let v: i64 = b0 as i64;
    v = v | ((b1 as i64) << 8);
    v = v | ((b2 as i64) << 16);
    v = v | ((b3 as i64) << 24);
    return v;
}

// 32 x 32 -> low 32 bits, without overflowing a signed 64-bit intermediate.
function mul32(a: i64, b: i64) => i64 {
    let lo: i64 = (a * (b & 65535)) & M32;
    let hi: i64 = (((a * (b >> 16)) & 65535) << 16) & M32;
    return (lo + hi) & M32;
}

// Unsigned 32 x 32 -> 64: returns the low word, the high word goes to MULHI.
let MULHI: i64 = 0;
function umul64(a: i64, b: i64) => i64 {
    let p0: i64 = a * (b & 65535);
    let p1: i64 = a * (b >> 16);
    let low: i64 = p0 + ((p1 & 65535) << 16);
    MULHI = ((p1 >> 16) + (low >> 32)) & M32;
    return low & M32;
}

struct GameBoyAdvance {
    flN: int = 0;
    flZ: int = 0;
    flC: int = 0;
    flV: int = 0;
    ctl: int = 0x1F;             // I (0x80), F (0x40), T (0x20), mode (0x1F)
    shc: int = 0;                // shifter carry out
    ac: int = 0;                 // carry / overflow of the last addc()
    av: int = 0;
    cyc: int = 0;                // cycles of the current step
    halted: bool = false;
    waiting: bool = false;       // inside IntrWait / VBlankIntrWait
    waitFlags: int = 0;
    iff: int = 0;                // IF register
    romLen: int = 0;
    lineCyc: int = 0;
    ly: int = 0;
    hbl: bool = false;
    frameDone: bool = false;
    keys: int = 0;               // 1 = down: 0 A, 1 B, 2 Select, 3 Start, 4 Right, 5 Left, 6 Up, 7 Down, 8 R, 9 L
    badOps: int = 0;             // undefined instructions seen (executed as NOP)
    lastBad: i64 = 0;
    swiMiss: int = 0;            // BIOS calls that are not implemented yet
    lastSwi: int = 0;
    battery: bool = false;
    ramBytes: int = 0;

    // ---------------------------------------------------------------- setup
    reset() => void {
        for (let i: int = 0; i < 16; i = i + 1) { R[i] = 0; }
        for (let i: int = 0; i < 6; i = i + 1) { BR13[i] = 0; BR14[i] = 0; BSPSR[i] = 0; }
        for (let i: int = 0; i < 5; i = i + 1) { USRHI[i] = 0; FIQHI[i] = 0; }
        for (let i: int = 0; i < 262144; i = i + 1) { GEWRAM[i] = 0 as u8; }
        for (let i: int = 0; i < 32768; i = i + 1) { GIWRAM[i] = 0 as u8; }
        for (let i: int = 0; i < 1024; i = i + 1) { GIO[i] = 0 as u8; GPAL[i] = 0 as u8; GOAM[i] = 0 as u8; }
        for (let i: int = 0; i < 98304; i = i + 1) { GVRAM[i] = 0 as u8; }
        for (let i: int = 0; i < 153600; i = i + 1) { GFB[i] = 0 as u8; }
        this.initTables();
        this.initBios();
        this.flN = 0; this.flZ = 0; this.flC = 0; this.flV = 0;
        this.ctl = 0x1F;
        // post-BIOS state: system mode, stacks as the real BIOS leaves them
        BR13[3] = 0x03007FE0;
        BR13[2] = 0x03007FA0;
        R[13] = 0x03007F00;
        R[15] = 0x08000000;
        this.halted = false;
        this.waiting = false;
        this.waitFlags = 0;
        this.iff = 0;
        this.lineCyc = 0;
        this.ly = 0;
        this.hbl = false;
        this.frameDone = false;
        this.badOps = 0;
        this.swiMiss = 0;
        GIO[0x130] = 0xFF as u8;
        GIO[0x131] = 0x03 as u8;
        GIO[0x088] = 0x00 as u8;
        GIO[0x089] = 0x02 as u8;     // SOUNDBIAS
        this.keys = 0;
    }

    initTables() => void {
        for (let i: int = 0; i < 16; i = i + 1) { W16[i] = 1; W32[i] = 1; }
        W16[2] = 3; W32[2] = 6;
        W32[5] = 2; W32[6] = 2;
        for (let i: int = 8; i < 14; i = i + 1) { W16[i] = 2; W32[i] = 4; }
        W16[14] = 5; W32[14] = 5; W16[15] = 5; W32[15] = 5;
    }

    // Tiny BIOS: the IRQ vector and handler, so games get the usual "call the routine at 0x03007FFC" behaviour.
    initBios() => void {
        for (let i: int = 0; i < 16384; i = i + 1) { GBIOS[i] = 0 as u8; }
        this.biosW(0x18, 0xEA000042);     // b 0x128
        this.biosW(0x128, 0xE92D500F);    // stmfd sp!, {r0-r3, r12, lr}
        this.biosW(0x12C, 0xE3A00301);    // mov r0, #0x04000000
        this.biosW(0x130, 0xE28FE000);    // add lr, pc, #0
        this.biosW(0x134, 0xE510F004);    // ldr pc, [r0, #-4]
        this.biosW(0x138, 0xE8BD500F);    // ldmfd sp!, {r0-r3, r12, lr}
        this.biosW(0x13C, 0xE25EF004);    // subs pc, lr, #4
    }

    biosW(o: int, v: i64) => void {
        GBIOS[o] = (v & 255) as u8;
        GBIOS[o + 1] = ((v >> 8) & 255) as u8;
        GBIOS[o + 2] = ((v >> 16) & 255) as u8;
        GBIOS[o + 3] = ((v >> 24) & 255) as u8;
    }

    romPtr() => u8* { return GROM; }

    // Called after the ROM bytes were read straight into romPtr().
    romReady(len: int) => bool {
        let ok: bool = false;
        if (len >= 192 && len <= 33554432) {
            this.romLen = len;
            this.reset();
            ok = true;
        }
        return ok;
    }

    title() => char* {
        // title field of the header, 12 bytes, printed as far as it is printable; the C string lives in GTMP
        let n: int = 0;
        let done: bool = false;
        for (let i: int = 0; i < 12; i = i + 1) {
            let ch: int = GROM[0xA0 + i];
            if (ch < 32 || ch > 126) { done = true; }
            if (!done) { GTMP[n] = ch as u8; n = n + 1; }
        }
        GTMP[n] = 0 as u8;
        return GTMP as char*;
    }

    framebuffer() => u8* { return GFB; }
    width() => int { return 240; }
    height() => int { return 160; }
    getPc() => i64 { return R[15]; }
    getReg(i: int) => i64 { return R[i & 15]; }

    // idx: 0 A, 1 B, 2 Select, 3 Start, 4 Right, 5 Left, 6 Up, 7 Down, 8 R, 9 L
    setKey(idx: int, down: bool) => void {
        let m: int = 1 << idx;
        if (down) { this.keys = this.keys | m; } else { this.keys = this.keys & (~m); }
        let inv: int = (~this.keys) & 0x3FF;
        GIO[0x130] = (inv & 255) as u8;
        GIO[0x131] = ((inv >> 8) & 3) as u8;
    }

    // ---------------------------------------------------------------- memory (no timing)
    vramOff(o: int) => int {
        let v: int = o & 131071;
        if (v >= 98304) { v = v - 32768; }
        return v;
    }

    get8(a: i64) => int {
        let reg: int = ((a >> 24) & 15) as int;
        let o: int = (a & 16777215) as int;
        let r: int = 0;
        if (reg == 0) {
            if (o < 16384) { r = GBIOS[o]; }
        } else if (reg == 2) {
            r = GEWRAM[o & 262143];
        } else if (reg == 3) {
            r = GIWRAM[o & 32767];
        } else if (reg == 4) {
            if (o < 1024) {
                r = GIO[o];
                if (o == 0x202) { r = this.iff & 255; }
                if (o == 0x203) { r = (this.iff >> 8) & 255; }
            }
        } else if (reg == 5) {
            r = GPAL[o & 1023];
        } else if (reg == 6) {
            r = GVRAM[this.vramOff(o)];
        } else if (reg == 7) {
            r = GOAM[o & 1023];
        } else if (reg >= 8 && reg <= 13) {
            let ro: int = (a & 33554431) as int;
            if (ro < this.romLen) {
                r = GROM[ro];
            } else {
                let hw: int = (ro >> 1) & 65535;
                if ((ro & 1) == 0) { r = hw & 255; } else { r = (hw >> 8) & 255; }
            }
        } else if (reg == 14 || reg == 15) {
            r = GSAVE[o & 65535];
        }
        return r;
    }

    get16(a: i64) => int {
        let aa: i64 = a & 4294967294;
        let reg: int = ((aa >> 24) & 15) as int;
        let r: int = 0;
        let fast: bool = false;
        if (reg >= 8 && reg <= 13) {
            let ro: int = (aa & 33554431) as int;
            if (ro + 1 < this.romLen) {
                let lo: int = GROM[ro];
                let hi: int = GROM[ro + 1];
                r = lo | (hi << 8);
                fast = true;
            }
        } else if (reg == 3) {
            let o3: int = (aa & 32767) as int;
            let lo3: int = GIWRAM[o3];
            let hi3: int = GIWRAM[o3 + 1];
            r = lo3 | (hi3 << 8);
            fast = true;
        }
        if (!fast) {
            let b0: int = this.get8(aa);
            let b1: int = this.get8(aa + 1);
            r = b0 | (b1 << 8);
        }
        return r;
    }

    get32(a: i64) => i64 {
        let aa: i64 = a & 4294967292;
        let reg: int = ((aa >> 24) & 15) as int;
        let r: i64 = 0;
        let fast: bool = false;
        if (reg >= 8 && reg <= 13) {
            let ro: int = (aa & 33554431) as int;
            if (ro + 3 < this.romLen) {
                r = mk32(GROM[ro], GROM[ro + 1], GROM[ro + 2], GROM[ro + 3]);
                fast = true;
            }
        } else if (reg == 3) {
            let o3: int = (aa & 32767) as int;
            r = mk32(GIWRAM[o3], GIWRAM[o3 + 1], GIWRAM[o3 + 2], GIWRAM[o3 + 3]);
            fast = true;
        }
        if (!fast) {
            let lo: int = this.get16(aa);
            let hi: int = this.get16(aa + 2);
            r = (lo as i64) | ((hi as i64) << 16);
        }
        return r;
    }

    wrIO8(o: int, v: int) => void {
        if (o >= 1024) { return; }
        if (o == 0x04) {
            let old: int = GIO[4];
            GIO[4] = ((old & 7) | (v & 0xF8)) as u8;
        } else if (o == 0x06 || o == 0x07) {
            // VCOUNT is read-only
        } else if (o == 0x130 || o == 0x131) {
            // KEYINPUT is read-only
        } else if (o == 0x202) {
            this.iff = this.iff & (~v);
        } else if (o == 0x203) {
            this.iff = this.iff & (~(v << 8));
        } else if (o == 0x301) {
            this.halted = true;
        } else {
            GIO[o] = v as u8;
        }
    }

    put8(a: i64, v: int) => void {
        let reg: int = ((a >> 24) & 15) as int;
        let o: int = (a & 16777215) as int;
        if (reg == 2) {
            GEWRAM[o & 262143] = v as u8;
        } else if (reg == 3) {
            GIWRAM[o & 32767] = v as u8;
        } else if (reg == 4) {
            this.wrIO8(o, v);
        } else if (reg == 5) {
            let p: int = o & 1022;
            GPAL[p] = v as u8;
            GPAL[p + 1] = v as u8;
        } else if (reg == 6) {
            let vo: int = this.vramOff(o);
            let lim: int = 65536;
            if ((GIO[0] & 7) >= 3) { lim = 81920; }
            if (vo < lim) {
                let ve: int = vo & 131070;
                GVRAM[ve] = v as u8;
                GVRAM[ve + 1] = v as u8;
            }
        } else if (reg == 14 || reg == 15) {
            GSAVE[o & 65535] = v as u8;
        }
    }

    put16(a: i64, v: int) => void {
        let aa: i64 = a & 4294967294;
        let reg: int = ((aa >> 24) & 15) as int;
        let o: int = (aa & 16777215) as int;
        let lo: int = v & 255;
        let hi: int = (v >> 8) & 255;
        if (reg == 2) {
            GEWRAM[o & 262143] = lo as u8;
            GEWRAM[(o & 262143) + 1] = hi as u8;
        } else if (reg == 3) {
            GIWRAM[o & 32767] = lo as u8;
            GIWRAM[(o & 32767) + 1] = hi as u8;
        } else if (reg == 4) {
            this.wrIO8(o, lo);
            this.wrIO8(o + 1, hi);
        } else if (reg == 5) {
            GPAL[o & 1022] = lo as u8;
            GPAL[(o & 1022) + 1] = hi as u8;
        } else if (reg == 6) {
            let vo: int = this.vramOff(o) & 131070;
            let lim: int = 65536;
            if ((GIO[0] & 7) >= 3) { lim = 81920; }
            GVRAM[vo] = lo as u8;
            GVRAM[vo + 1] = hi as u8;
        } else if (reg == 7) {
            GOAM[o & 1022] = lo as u8;
            GOAM[(o & 1022) + 1] = hi as u8;
        } else if (reg == 14 || reg == 15) {
            GSAVE[o & 65535] = lo as u8;
        }
    }

    // ---------------------------------------------------------------- memory (with timing)
    rd8(a: i64) => int {
        this.cyc = this.cyc + W16[((a >> 24) & 15) as int];
        return this.get8(a);
    }

    rd16(a: i64) => int {
        this.cyc = this.cyc + W16[((a >> 24) & 15) as int];
        return this.get16(a);
    }

    rd32(a: i64) => i64 {
        this.cyc = this.cyc + W32[((a >> 24) & 15) as int];
        return this.get32(a);
    }

    wr8(a: i64, v: int) => void {
        this.cyc = this.cyc + W16[((a >> 24) & 15) as int];
        this.put8(a, v & 255);
    }

    wr16(a: i64, v: int) => void {
        this.cyc = this.cyc + W16[((a >> 24) & 15) as int];
        this.put16(a, v & 65535);
    }

    wr32(a: i64, v: i64) => void {
        this.cyc = this.cyc + W32[((a >> 24) & 15) as int];
        let aa: i64 = a & 4294967292;
        this.put16(aa, (v & 65535) as int);
        this.put16(aa + 2, ((v >> 16) & 65535) as int);
    }

    ldr32(a: i64) => i64 {
        let v: i64 = this.rd32(a);
        return ror32(v, ((a & 3) * 8) as int);
    }

    ldrh(a: i64) => i64 {
        let v: i64 = this.rd16(a) as i64;
        if ((a & 1) != 0) { v = ror32(v, 8); }
        return v;
    }

    ldrsb(a: i64) => i64 {
        let b: int = this.rd8(a);
        let v: i64 = b as i64;
        if (b >= 128) { v = (v - 256) & M32; }
        return v;
    }

    ldrsh(a: i64) => i64 {
        let v: i64 = 0;
        if ((a & 1) != 0) {
            v = this.ldrsb(a);
        } else {
            let h: int = this.rd16(a);
            v = h as i64;
            if (h >= 32768) { v = (v - 65536) & M32; }
        }
        return v;
    }

    // ---------------------------------------------------------------- registers and PSR
    isThumb() => bool { return (this.ctl & 0x20) != 0; }

    rdOp(n: int) => i64 {
        let v: i64 = R[n];
        if (n == 15) {
            if ((this.ctl & 0x20) != 0) { v = (v + 2) & M32; } else { v = (v + 4) & M32; }
        }
        return v;
    }

    setReg(n: int, v: i64) => void {
        if (n == 15) {
            if ((this.ctl & 0x20) != 0) { R[15] = v & 4294967294; } else { R[15] = v & 4294967292; }
        } else {
            R[n] = v & M32;
        }
    }

    getCpsr() => i64 {
        let v: i64 = ((this.flN as i64) << 31) | ((this.flZ as i64) << 30) | ((this.flC as i64) << 29) | ((this.flV as i64) << 28);
        return v | (this.ctl as i64);
    }

    setFlags(v: i64) => void {
        this.flN = ((v >> 31) & 1) as int;
        this.flZ = ((v >> 30) & 1) as int;
        this.flC = ((v >> 29) & 1) as int;
        this.flV = ((v >> 28) & 1) as int;
    }

    switchMode(nm: int) => void {
        let om: int = this.ctl & 0x1F;
        let ob: int = bankOf(om);
        let nb: int = bankOf(nm);
        if (ob != nb) {
            BR13[ob] = R[13];
            BR14[ob] = R[14];
            if (ob == 1) {
                for (let i: int = 0; i < 5; i = i + 1) { FIQHI[i] = R[8 + i]; R[8 + i] = USRHI[i]; }
            }
            if (nb == 1) {
                for (let i: int = 0; i < 5; i = i + 1) { USRHI[i] = R[8 + i]; R[8 + i] = FIQHI[i]; }
            }
            R[13] = BR13[nb];
            R[14] = BR14[nb];
        }
        this.ctl = (this.ctl & 0xE0) | (nm & 0x1F);
    }

    // CPSR = SPSR of the current mode (return from exception)
    restoreSpsr() => void {
        let b: int = bankOf(this.ctl & 0x1F);
        if (b != 0) {
            let s: i64 = BSPSR[b];
            this.setFlags(s);
            this.switchMode((s & 31) as int);
            this.ctl = (s & 255) as int;
        }
    }

    // MSR: fm = field mask (bit 3 flags, bit 0 control), val = value to write
    msr(useSpsr: bool, fm: int, val: i64) => void {
        if (useSpsr) {
            let b: int = bankOf(this.ctl & 0x1F);
            if (b != 0) {
                let s: i64 = BSPSR[b];
                if ((fm & 8) != 0) { s = (s & 268435455) | (val & 4026531840); }
                if ((fm & 1) != 0) { s = (s & 4294967040) | (val & 255); }
                BSPSR[b] = s;
            }
        } else {
            if ((fm & 8) != 0) { this.setFlags(val); }
            if ((fm & 1) != 0 && (this.ctl & 0x1F) != 0x10) {
                let nm: int = (val & 31) as int;
                if (nm != (this.ctl & 0x1F)) { this.switchMode(nm); }
                this.ctl = (this.ctl & 0x1F) | ((val & 0xC0) as int) | (this.ctl & 0x20);
            }
        }
    }

    cond(c: int) => bool {
        let r: bool = true;
        switch (c) {
            case 0: r = this.flZ == 1; break;
            case 1: r = this.flZ == 0; break;
            case 2: r = this.flC == 1; break;
            case 3: r = this.flC == 0; break;
            case 4: r = this.flN == 1; break;
            case 5: r = this.flN == 0; break;
            case 6: r = this.flV == 1; break;
            case 7: r = this.flV == 0; break;
            case 8: r = (this.flC == 1 && this.flZ == 0); break;
            case 9: r = (this.flC == 0 || this.flZ == 1); break;
            case 10: r = this.flN == this.flV; break;
            case 11: r = this.flN != this.flV; break;
            case 12: r = (this.flZ == 0 && this.flN == this.flV); break;
            case 13: r = (this.flZ == 1 || this.flN != this.flV); break;
            case 14: r = true; break;
            default: r = false; break;
        }
        return r;
    }

    // ---------------------------------------------------------------- ALU helpers
    // a + b + c as 32-bit; leaves carry and overflow in ac / av.
    addc(a: i64, b: i64, c: int) => i64 {
        let sum: i64 = a + b + (c as i64);
        let res: i64 = sum & M32;
        this.ac = 0;
        if (sum > M32) { this.ac = 1; }
        this.av = ((((a ^ res) & (b ^ res)) >> 31) & 1) as int;
        return res;
    }

    setNZ(v: i64) => void {
        this.flN = ((v >> 31) & 1) as int;
        this.flZ = 0;
        if (v == 0) { this.flZ = 1; }
    }

    // amt >= 1 is the real shift amount here. Result in the return value, carry out in shc.
    shiftCore(st: int, v: i64, amt: int) => i64 {
        let r: i64 = 0;
        if (st == 0) {
            if (amt < 32) {
                r = (v << amt) & M32;
                this.shc = ((v >> (32 - amt)) & 1) as int;
            } else if (amt == 32) {
                r = 0;
                this.shc = (v & 1) as int;
            } else {
                r = 0;
                this.shc = 0;
            }
        } else if (st == 1) {
            if (amt < 32) {
                r = v >> amt;
                this.shc = ((v >> (amt - 1)) & 1) as int;
            } else if (amt == 32) {
                r = 0;
                this.shc = ((v >> 31) & 1) as int;
            } else {
                r = 0;
                this.shc = 0;
            }
        } else if (st == 2) {
            let neg: bool = ((v >> 31) & 1) != 0;
            if (amt < 32) {
                r = (s32(v) >> amt) & M32;
                this.shc = ((v >> (amt - 1)) & 1) as int;
            } else {
                if (neg) { r = M32; } else { r = 0; }
                this.shc = ((v >> 31) & 1) as int;
            }
        } else {
            let k: int = amt & 31;
            if (k == 0) {
                r = v;
                this.shc = ((v >> 31) & 1) as int;
            } else {
                r = ror32(v, k);
                this.shc = ((r >> 31) & 1) as int;
            }
        }
        return r;
    }

    // Immediate-encoded shifts (amount 0 has special meanings).
    shiftImm(st: int, v: i64, amt: int) => i64 {
        let r: i64 = v;
        this.shc = this.flC;
        if (amt == 0) {
            if (st == 0) {
                r = v;
            } else if (st == 1 || st == 2) {
                r = this.shiftCore(st, v, 32);
            } else {
                r = ((v >> 1) | ((this.flC as i64) << 31)) & M32;
                this.shc = (v & 1) as int;
            }
        } else {
            r = this.shiftCore(st, v, amt);
        }
        return r;
    }

    // Register-specified shifts (amount 0 leaves value and carry alone).
    shiftReg(st: int, v: i64, amt: int) => i64 {
        let r: i64 = v;
        this.shc = this.flC;
        if (amt != 0) { r = this.shiftCore(st, v, amt); }
        return r;
    }

    // ---------------------------------------------------------------- ARM
    execArm(op: i64) => void {
        let cc: int = ((op >> 28) & 15) as int;
        if (!this.cond(cc)) { return; }
        let top: int = ((op >> 25) & 7) as int;
        if (top == 0) {
            if ((op & 0xF0) == 0x90) {
                if ((op & 0x0FC00000) == 0) {
                    this.mulArm(op);
                } else if ((op & 0x0F800000) == 0x00800000) {
                    this.mullArm(op);
                } else if ((op & 0x0FB00000) == 0x01000000) {
                    this.swpArm(op);
                } else {
                    this.undef(op);
                }
            } else if ((op & 0x90) == 0x90) {
                this.hwArm(op);
            } else if ((op & 0x0FFFFFF0) == 0x012FFF10) {
                let v: i64 = this.rdOp((op & 15) as int);
                if ((v & 1) != 0) {
                    this.ctl = this.ctl | 0x20;
                    R[15] = v & 4294967294;
                } else {
                    R[15] = v & 4294967292;
                }
            } else {
                let opc: int = ((op >> 21) & 15) as int;
                let s: int = ((op >> 20) & 1) as int;
                if (opc >= 8 && opc <= 11 && s == 0) {
                    this.psrArm(op);
                } else {
                    this.dataProc(op);
                }
            }
        } else if (top == 1) {
            let opc: int = ((op >> 21) & 15) as int;
            let s: int = ((op >> 20) & 1) as int;
            if (opc >= 8 && opc <= 11 && s == 0) {
                this.psrArm(op);
            } else {
                this.dataProc(op);
            }
        } else if (top == 2) {
            this.ldst(op);
        } else if (top == 3) {
            if ((op & 16) != 0) { this.undef(op); } else { this.ldst(op); }
        } else if (top == 4) {
            this.ldm(op);
        } else if (top == 5) {
            let off: i64 = op & 16777215;
            if (off >= 8388608) { off = off - 16777216; }
            if (((op >> 24) & 1) != 0) { R[14] = R[15] & M32; }
            R[15] = (R[15] + 4 + (off << 2)) & 4294967292;
            this.cyc = this.cyc + 3;
        } else if (top == 7) {
            if (((op >> 24) & 1) != 0) { this.swi(((op >> 16) & 255) as int); } else { this.undef(op); }
        } else {
            this.undef(op);
        }
    }

    undef(op: i64) => void {
        this.badOps = this.badOps + 1;
        this.lastBad = op;
    }

    dataProc(op: i64) => void {
        let opc: int = ((op >> 21) & 15) as int;
        let s: int = ((op >> 20) & 1) as int;
        let rn: int = ((op >> 16) & 15) as int;
        let rd: int = ((op >> 12) & 15) as int;
        let b: i64 = 0;
        let regShift: bool = false;
        this.shc = this.flC;
        if (((op >> 25) & 1) != 0) {
            let rot: int = ((((op >> 8) & 15)) * 2) as int;
            b = ror32(op & 255, rot);
            if (rot != 0) { this.shc = ((b >> 31) & 1) as int; }
        } else {
            let rm: int = (op & 15) as int;
            let st: int = ((op >> 5) & 3) as int;
            let v: i64 = this.rdOp(rm);
            if (((op >> 4) & 1) != 0) {
                regShift = true;
                let rs: int = ((op >> 8) & 15) as int;
                let amt: int = (this.rdOp(rs) & 255) as int;
                if (rm == 15) { v = (v + 4) & M32; }
                b = this.shiftReg(st, v, amt);
                this.cyc = this.cyc + 1;
            } else {
                b = this.shiftImm(st, v, ((op >> 7) & 31) as int);
            }
        }
        let a: i64 = this.rdOp(rn);
        if (regShift && rn == 15) { a = (a + 4) & M32; }
        let r: i64 = 0;
        let write: bool = true;
        let logical: bool = false;
        switch (opc) {
            case 0: r = a & b; logical = true; break;
            case 1: r = a ^ b; logical = true; break;
            case 2: r = this.addc(a, (~b) & M32, 1); break;
            case 3: r = this.addc(b, (~a) & M32, 1); break;
            case 4: r = this.addc(a, b, 0); break;
            case 5: r = this.addc(a, b, this.flC); break;
            case 6: r = this.addc(a, (~b) & M32, this.flC); break;
            case 7: r = this.addc(b, (~a) & M32, this.flC); break;
            case 8: r = a & b; logical = true; write = false; break;
            case 9: r = a ^ b; logical = true; write = false; break;
            case 10: r = this.addc(a, (~b) & M32, 1); write = false; break;
            case 11: r = this.addc(a, b, 0); write = false; break;
            case 12: r = a | b; logical = true; break;
            case 13: r = b; logical = true; break;
            case 14: r = a & ((~b) & M32); logical = true; break;
            default: r = (~b) & M32; logical = true; break;
        }
        r = r & M32;
        if (s != 0 && !(rd == 15 && write)) {
            this.setNZ(r);
            if (logical) {
                this.flC = this.shc;
            } else {
                this.flC = this.ac;
                this.flV = this.av;
            }
        }
        if (write) {
            if (rd == 15) {
                if (s != 0) {
                    this.restoreSpsr();
                    this.setReg(15, r);
                } else {
                    R[15] = r & 4294967292;
                }
                this.cyc = this.cyc + 3;
            } else {
                R[rd] = r;
            }
        }
    }

    psrArm(op: i64) => void {
        let useSpsr: bool = ((op >> 22) & 1) != 0;
        if ((op & 0x0FBF0FFF) == 0x010F0000) {
            let rd: int = ((op >> 12) & 15) as int;
            let v: i64 = 0;
            if (useSpsr) {
                let b: int = bankOf(this.ctl & 0x1F);
                if (b != 0) { v = BSPSR[b]; } else { v = this.getCpsr(); }
            } else {
                v = this.getCpsr();
            }
            R[rd] = v & M32;
        } else if ((op & 0x0FB0F000) == 0x0120F000 || (op & 0x0FB0F000) == 0x0320F000) {
            let fm: int = ((op >> 16) & 15) as int;
            let v: i64 = 0;
            if (((op >> 25) & 1) != 0) {
                v = ror32(op & 255, ((((op >> 8) & 15)) * 2) as int);
            } else {
                v = this.rdOp((op & 15) as int);
            }
            this.msr(useSpsr, fm, v);
        } else {
            this.undef(op);
        }
    }

    mulArm(op: i64) => void {
        let rd: int = ((op >> 16) & 15) as int;
        let rn: int = ((op >> 12) & 15) as int;
        let rs: int = ((op >> 8) & 15) as int;
        let rm: int = (op & 15) as int;
        let r: i64 = mul32(R[rm], R[rs]);
        if (((op >> 21) & 1) != 0) { r = (r + R[rn]) & M32; }
        R[rd] = r;
        if (((op >> 20) & 1) != 0) { this.setNZ(r); }
        this.cyc = this.cyc + 3;
    }

    mullArm(op: i64) => void {
        let rdh: int = ((op >> 16) & 15) as int;
        let rdl: int = ((op >> 12) & 15) as int;
        let rs: int = ((op >> 8) & 15) as int;
        let rm: int = (op & 15) as int;
        let signed: bool = ((op >> 22) & 1) != 0;
        let acc: bool = ((op >> 21) & 1) != 0;
        let a: i64 = R[rm];
        let b: i64 = R[rs];
        let lo: i64 = umul64(a, b);
        let hi: i64 = MULHI;
        if (signed) {
            if (a >= B31) { hi = (hi - b) & M32; }
            if (b >= B31) { hi = (hi - a) & M32; }
        }
        if (acc) {
            let nl: i64 = lo + R[rdl];
            hi = (hi + R[rdh] + (nl >> 32)) & M32;
            lo = nl & M32;
        }
        R[rdl] = lo;
        R[rdh] = hi;
        if (((op >> 20) & 1) != 0) {
            this.flN = ((hi >> 31) & 1) as int;
            this.flZ = 0;
            if (lo == 0 && hi == 0) { this.flZ = 1; }
        }
        this.cyc = this.cyc + 4;
    }

    swpArm(op: i64) => void {
        let rn: int = ((op >> 16) & 15) as int;
        let rd: int = ((op >> 12) & 15) as int;
        let rm: int = (op & 15) as int;
        let addr: i64 = R[rn];
        let src: i64 = R[rm];
        if (((op >> 22) & 1) != 0) {
            let t: int = this.rd8(addr);
            this.wr8(addr, (src & 255) as int);
            R[rd] = t as i64;
        } else {
            let t: i64 = this.ldr32(addr);
            this.wr32(addr, src);
            R[rd] = t;
        }
    }

    ldst(op: i64) => void {
        let pre: int = ((op >> 24) & 1) as int;
        let up: int = ((op >> 23) & 1) as int;
        let byt: int = ((op >> 22) & 1) as int;
        let wb: int = ((op >> 21) & 1) as int;
        let ld: int = ((op >> 20) & 1) as int;
        let rn: int = ((op >> 16) & 15) as int;
        let rd: int = ((op >> 12) & 15) as int;
        let off: i64 = 0;
        if (((op >> 25) & 1) != 0) {
            off = this.shiftImm(((op >> 5) & 3) as int, this.rdOp((op & 15) as int), ((op >> 7) & 31) as int);
        } else {
            off = op & 4095;
        }
        let base: i64 = this.rdOp(rn);
        let nb: i64 = 0;
        if (up != 0) { nb = (base + off) & M32; } else { nb = (base - off) & M32; }
        let addr: i64 = base;
        if (pre != 0) { addr = nb; }
        if (ld != 0) {
            let v: i64 = 0;
            if (byt != 0) { v = this.rd8(addr) as i64; } else { v = this.ldr32(addr); }
            if (pre == 0 || wb != 0) { R[rn] = nb; }
            this.setReg(rd, v);
            this.cyc = this.cyc + 1;
        } else {
            let sv: i64 = this.rdOp(rd);
            if (rd == 15) { sv = (sv + 4) & M32; }
            if (byt != 0) { this.wr8(addr, (sv & 255) as int); } else { this.wr32(addr, sv); }
            if (pre == 0 || wb != 0) { R[rn] = nb; }
        }
    }

    hwArm(op: i64) => void {
        let pre: int = ((op >> 24) & 1) as int;
        let up: int = ((op >> 23) & 1) as int;
        let imm: int = ((op >> 22) & 1) as int;
        let wb: int = ((op >> 21) & 1) as int;
        let ld: int = ((op >> 20) & 1) as int;
        let rn: int = ((op >> 16) & 15) as int;
        let rd: int = ((op >> 12) & 15) as int;
        let sh: int = ((op >> 5) & 3) as int;
        let off: i64 = 0;
        if (imm != 0) { off = ((op >> 4) & 0xF0) | (op & 15); } else { off = this.rdOp((op & 15) as int); }
        let base: i64 = this.rdOp(rn);
        let nb: i64 = 0;
        if (up != 0) { nb = (base + off) & M32; } else { nb = (base - off) & M32; }
        let addr: i64 = base;
        if (pre != 0) { addr = nb; }
        if (ld != 0) {
            let v: i64 = 0;
            if (sh == 1) { v = this.ldrh(addr); }
            else if (sh == 2) { v = this.ldrsb(addr); }
            else { v = this.ldrsh(addr); }
            if (pre == 0 || wb != 0) { R[rn] = nb; }
            this.setReg(rd, v);
            this.cyc = this.cyc + 1;
        } else {
            let sv: i64 = this.rdOp(rd);
            if (rd == 15) { sv = (sv + 4) & M32; }
            this.wr16(addr, (sv & 65535) as int);
            if (pre == 0 || wb != 0) { R[rn] = nb; }
        }
    }

    ldm(op: i64) => void {
        let pre: int = ((op >> 24) & 1) as int;
        let up: int = ((op >> 23) & 1) as int;
        let sbit: int = ((op >> 22) & 1) as int;
        let wb: int = ((op >> 21) & 1) as int;
        let ld: int = ((op >> 20) & 1) as int;
        let rn: int = ((op >> 16) & 15) as int;
        let rl: int = (op & 65535) as int;
        let base: i64 = R[rn];
        let n: int = 0;
        for (let i: int = 0; i < 16; i = i + 1) { if ((rl & (1 << i)) != 0) { n = n + 1; } }
        let empty: bool = false;
        if (rl == 0) { rl = 32768; n = 16; empty = true; }
        let start: i64 = base;
        let wbv: i64 = base;
        if (up != 0) {
            wbv = (base + (n * 4)) & M32;
            if (pre != 0) { start = (base + 4) & M32; }
        } else {
            wbv = (base - (n * 4)) & M32;
            start = wbv;
            if (pre == 0) { start = (start + 4) & M32; }
        }
        let userBank: bool = false;
        let om: int = this.ctl & 0x1F;
        if (sbit != 0 && !(ld != 0 && (rl & 32768) != 0)) { userBank = true; }
        if (userBank) { this.switchMode(0x10); }
        let addr: i64 = start;
        if (ld != 0) {
            let pcv: i64 = 0;
            let loadedPc: bool = false;
            for (let i: int = 0; i < 16; i = i + 1) {
                if ((rl & (1 << i)) != 0) {
                    let v: i64 = this.rd32(addr);
                    if (i == 15) { pcv = v; loadedPc = true; } else { R[i] = v; }
                    addr = (addr + 4) & M32;
                }
            }
            if (userBank) { this.switchMode(om); }
            if (wb != 0 && (rl & (1 << rn)) == 0) { R[rn] = wbv; }
            if (empty) { if (wb != 0) { R[rn] = wbv; } }
            if (loadedPc) {
                if (sbit != 0) {
                    this.restoreSpsr();
                    this.setReg(15, pcv);
                } else {
                    R[15] = pcv & 4294967292;
                }
            }
            this.cyc = this.cyc + 2;
        } else {
            let first: bool = true;
            for (let i: int = 0; i < 16; i = i + 1) {
                if ((rl & (1 << i)) != 0) {
                    let v: i64 = R[i];
                    if (i == 15) { v = (R[15] + 8) & M32; }
                    if (i == rn && !first && !userBank) { v = wbv; }
                    this.wr32(addr, v);
                    addr = (addr + 4) & M32;
                    first = false;
                }
            }
            if (userBank) { this.switchMode(om); }
            if (wb != 0) { R[rn] = wbv; }
            this.cyc = this.cyc + 1;
        }
    }

    // ---------------------------------------------------------------- THUMB
    execThumb(op: int) => void {
        let hi5: int = op >> 11;
        if (hi5 < 3) {
            let rd: int = op & 7;
            let rs: int = (op >> 3) & 7;
            let r: i64 = this.shiftImm(hi5, R[rs], (op >> 6) & 31);
            R[rd] = r;
            this.setNZ(r);
            this.flC = this.shc;
        } else if (hi5 == 3) {
            let rd: int = op & 7;
            let rs: int = (op >> 3) & 7;
            let rn: int = (op >> 6) & 7;
            let b: i64 = rn as i64;
            if (((op >> 10) & 1) == 0) { b = R[rn]; }
            let r: i64 = 0;
            if (((op >> 9) & 1) != 0) { r = this.addc(R[rs], (~b) & M32, 1); } else { r = this.addc(R[rs], b, 0); }
            R[rd] = r;
            this.setNZ(r);
            this.flC = this.ac;
            this.flV = this.av;
        } else if (hi5 <= 7) {
            let rd: int = (op >> 8) & 7;
            let imm: i64 = (op & 255) as i64;
            let sub: int = hi5 - 4;
            let r: i64 = 0;
            if (sub == 0) {
                r = imm;
                R[rd] = r;
                this.setNZ(r);
            } else if (sub == 1) {
                r = this.addc(R[rd], (~imm) & M32, 1);
                this.setNZ(r);
                this.flC = this.ac;
                this.flV = this.av;
            } else if (sub == 2) {
                r = this.addc(R[rd], imm, 0);
                R[rd] = r;
                this.setNZ(r);
                this.flC = this.ac;
                this.flV = this.av;
            } else {
                r = this.addc(R[rd], (~imm) & M32, 1);
                R[rd] = r;
                this.setNZ(r);
                this.flC = this.ac;
                this.flV = this.av;
            }
        } else if (hi5 == 8) {
            if ((op >> 10) == 0x10) {
                this.thumbAlu(op);
            } else {
                this.thumbHi(op);
            }
        } else if (hi5 == 9) {
            let rd: int = (op >> 8) & 7;
            let addr: i64 = (((R[15] + 2) & 4294967292) + ((op & 255) * 4)) & M32;
            R[rd] = this.rd32(addr);
            this.cyc = this.cyc + 1;
        } else if (hi5 == 10 || hi5 == 11) {
            this.thumbRegOff(op);
        } else if (hi5 >= 12 && hi5 <= 15) {
            let rd: int = op & 7;
            let rb: int = (op >> 3) & 7;
            let off: int = (op >> 6) & 31;
            let kind: int = hi5 - 12;
            if (kind == 0) {
                this.wr32((R[rb] + (off * 4)) & M32, R[rd]);
            } else if (kind == 1) {
                R[rd] = this.ldr32((R[rb] + (off * 4)) & M32);
                this.cyc = this.cyc + 1;
            } else if (kind == 2) {
                this.wr8((R[rb] + off) & M32, (R[rd] & 255) as int);
            } else {
                R[rd] = this.rd8((R[rb] + off) & M32) as i64;
                this.cyc = this.cyc + 1;
            }
        } else if (hi5 == 16) {
            let rd: int = op & 7;
            let rb: int = (op >> 3) & 7;
            this.wr16((R[rb] + (((op >> 6) & 31) * 2)) & M32, (R[rd] & 65535) as int);
        } else if (hi5 == 17) {
            let rd: int = op & 7;
            let rb: int = (op >> 3) & 7;
            R[rd] = this.ldrh((R[rb] + (((op >> 6) & 31) * 2)) & M32);
            this.cyc = this.cyc + 1;
        } else if (hi5 == 18) {
            let rd: int = (op >> 8) & 7;
            this.wr32((R[13] + ((op & 255) * 4)) & M32, R[rd]);
        } else if (hi5 == 19) {
            let rd: int = (op >> 8) & 7;
            R[rd] = this.ldr32((R[13] + ((op & 255) * 4)) & M32);
            this.cyc = this.cyc + 1;
        } else if (hi5 == 20) {
            let rd: int = (op >> 8) & 7;
            R[rd] = (((R[15] + 2) & 4294967292) + ((op & 255) * 4)) & M32;
        } else if (hi5 == 21) {
            let rd: int = (op >> 8) & 7;
            R[rd] = (R[13] + ((op & 255) * 4)) & M32;
        } else if (hi5 == 22 || hi5 == 23) {
            this.thumbMisc(op);
        } else if (hi5 == 24 || hi5 == 25) {
            this.thumbBlock(op);
        } else if (hi5 == 26 || hi5 == 27) {
            let c: int = (op >> 8) & 15;
            if (c == 15) {
                this.swi(op & 255);
            } else if (c == 14) {
                this.undef(op as i64);
            } else if (this.cond(c)) {
                let off: int = op & 255;
                if (off >= 128) { off = off - 256; }
                R[15] = (R[15] + 2 + (off * 2)) & M32;
                this.cyc = this.cyc + 3;
            }
        } else if (hi5 == 28) {
            let off: int = op & 2047;
            if (off >= 1024) { off = off - 2048; }
            R[15] = (R[15] + 2 + (off * 2)) & M32;
            this.cyc = this.cyc + 3;
        } else if (hi5 == 30) {
            let off: int = op & 2047;
            if (off >= 1024) { off = off - 2048; }
            R[14] = (R[15] + 2 + ((off as i64) << 12)) & M32;
        } else if (hi5 == 31) {
            let off: i64 = (op & 2047) as i64;
            let t: i64 = (R[14] + (off << 1)) & M32;
            R[14] = R[15] | 1;
            R[15] = t & 4294967294;
            this.cyc = this.cyc + 3;
        } else {
            this.undef(op as i64);
        }
    }

    thumbAlu(op: int) => void {
        let rd: int = op & 7;
        let rs: int = (op >> 3) & 7;
        let a: i64 = R[rd];
        let b: i64 = R[rs];
        let code: int = (op >> 6) & 15;
        let r: i64 = 0;
        let write: bool = true;
        let arith: bool = false;
        let shift: bool = false;
        switch (code) {
            case 0: r = a & b; break;
            case 1: r = a ^ b; break;
            case 2: r = this.shiftReg(0, a, (b & 255) as int); shift = true; break;
            case 3: r = this.shiftReg(1, a, (b & 255) as int); shift = true; break;
            case 4: r = this.shiftReg(2, a, (b & 255) as int); shift = true; break;
            case 5: r = this.addc(a, b, this.flC); arith = true; break;
            case 6: r = this.addc(a, (~b) & M32, this.flC); arith = true; break;
            case 7: r = this.shiftReg(3, a, (b & 255) as int); shift = true; break;
            case 8: r = a & b; write = false; break;
            case 9: r = this.addc(0, (~b) & M32, 1); arith = true; break;
            case 10: r = this.addc(a, (~b) & M32, 1); arith = true; write = false; break;
            case 11: r = this.addc(a, b, 0); arith = true; write = false; break;
            case 12: r = a | b; break;
            case 13: r = mul32(a, b); this.cyc = this.cyc + 3; break;
            case 14: r = a & ((~b) & M32); break;
            default: r = (~b) & M32; break;
        }
        r = r & M32;
        this.setNZ(r);
        if (arith) {
            this.flC = this.ac;
            this.flV = this.av;
        }
        if (shift) {
            this.flC = this.shc;
            this.cyc = this.cyc + 1;
        }
        if (write) { R[rd] = r; }
    }

    thumbHi(op: int) => void {
        let rd: int = (op & 7) | ((op >> 4) & 8);
        let rs: int = (op >> 3) & 15;
        let code: int = (op >> 8) & 3;
        let b: i64 = this.rdOp(rs);
        if (code == 0) {
            let r: i64 = (this.rdOp(rd) + b) & M32;
            if (rd == 15) { R[15] = r & 4294967294; this.cyc = this.cyc + 3; } else { R[rd] = r; }
        } else if (code == 1) {
            let r: i64 = this.addc(this.rdOp(rd), (~b) & M32, 1);
            this.setNZ(r);
            this.flC = this.ac;
            this.flV = this.av;
        } else if (code == 2) {
            if (rd == 15) { R[15] = b & 4294967294; this.cyc = this.cyc + 3; } else { R[rd] = b; }
        } else {
            if ((b & 1) != 0) {
                R[15] = b & 4294967294;
            } else {
                this.ctl = this.ctl & (~0x20);
                R[15] = b & 4294967292;
            }
            this.cyc = this.cyc + 3;
        }
    }

    thumbRegOff(op: int) => void {
        let rd: int = op & 7;
        let rb: int = (op >> 3) & 7;
        let ro: int = (op >> 6) & 7;
        let kind: int = (op >> 9) & 7;
        let addr: i64 = (R[rb] + R[ro]) & M32;
        if (kind == 0) {
            this.wr32(addr, R[rd]);
        } else if (kind == 1) {
            this.wr16(addr, (R[rd] & 65535) as int);
        } else if (kind == 2) {
            this.wr8(addr, (R[rd] & 255) as int);
        } else if (kind == 3) {
            R[rd] = this.ldrsb(addr);
            this.cyc = this.cyc + 1;
        } else if (kind == 4) {
            R[rd] = this.ldr32(addr);
            this.cyc = this.cyc + 1;
        } else if (kind == 5) {
            R[rd] = this.ldrh(addr);
            this.cyc = this.cyc + 1;
        } else if (kind == 6) {
            R[rd] = this.rd8(addr) as i64;
            this.cyc = this.cyc + 1;
        } else {
            R[rd] = this.ldrsh(addr);
            this.cyc = this.cyc + 1;
        }
    }

    thumbMisc(op: int) => void {
        let top: int = op >> 8;
        if (top == 0xB0) {
            let imm: i64 = ((op & 127) * 4) as i64;
            if ((op & 128) != 0) { R[13] = (R[13] - imm) & M32; } else { R[13] = (R[13] + imm) & M32; }
        } else if (top == 0xB4 || top == 0xB5) {
            let rl: int = op & 255;
            let n: int = 0;
            for (let i: int = 0; i < 8; i = i + 1) { if ((rl & (1 << i)) != 0) { n = n + 1; } }
            if (top == 0xB5) { n = n + 1; }
            let sp: i64 = (R[13] - (n * 4)) & M32;
            let addr: i64 = sp;
            for (let i: int = 0; i < 8; i = i + 1) {
                if ((rl & (1 << i)) != 0) {
                    this.wr32(addr, R[i]);
                    addr = (addr + 4) & M32;
                }
            }
            if (top == 0xB5) { this.wr32(addr, R[14]); }
            R[13] = sp;
        } else if (top == 0xBC || top == 0xBD) {
            let rl: int = op & 255;
            let addr: i64 = R[13];
            for (let i: int = 0; i < 8; i = i + 1) {
                if ((rl & (1 << i)) != 0) {
                    R[i] = this.rd32(addr);
                    addr = (addr + 4) & M32;
                }
            }
            if (top == 0xBD) {
                let v: i64 = this.rd32(addr);
                addr = (addr + 4) & M32;
                R[15] = v & 4294967294;
                this.cyc = this.cyc + 3;
            }
            R[13] = addr;
            this.cyc = this.cyc + 1;
        } else {
            this.undef(op as i64);
        }
    }

    thumbBlock(op: int) => void {
        let rb: int = (op >> 8) & 7;
        let rl: int = op & 255;
        let ld: bool = ((op >> 11) & 1) != 0;
        let addr: i64 = R[rb];
        let n: int = 0;
        for (let i: int = 0; i < 8; i = i + 1) { if ((rl & (1 << i)) != 0) { n = n + 1; } }
        if (rl == 0) {
            if (ld) {
                R[15] = this.rd32(addr) & 4294967294;
            } else {
                this.wr32(addr, (R[15] + 4) & M32);
            }
            R[rb] = (addr + 64) & M32;
        } else if (ld) {
            for (let i: int = 0; i < 8; i = i + 1) {
                if ((rl & (1 << i)) != 0) {
                    R[i] = this.rd32(addr);
                    addr = (addr + 4) & M32;
                }
            }
            if ((rl & (1 << rb)) == 0) { R[rb] = addr; }
            this.cyc = this.cyc + 1;
        } else {
            let first: bool = true;
            let nbase: i64 = (addr + (n * 4)) & M32;
            for (let i: int = 0; i < 8; i = i + 1) {
                if ((rl & (1 << i)) != 0) {
                    let v: i64 = R[i];
                    if (i == rb && !first) { v = nbase; }
                    this.wr32(addr, v);
                    addr = (addr + 4) & M32;
                    first = false;
                }
            }
            R[rb] = nbase;
        }
    }

    // ---------------------------------------------------------------- BIOS (high level)
    swi(n: int) => void {
        this.cyc = this.cyc + 20;
        if (n == 0x01) {
            this.biosRamReset((R[0] & 255) as int);
        } else if (n == 0x02 || n == 0x03) {
            this.halted = true;
        } else if (n == 0x04) {
            this.intrWait((R[0] & 1) as int, (R[1] & 16383) as int);
        } else if (n == 0x05) {
            this.intrWait(1, 1);
        } else if (n == 0x06) {
            this.biosDiv(R[0], R[1]);
        } else if (n == 0x07) {
            this.biosDiv(R[1], R[0]);
        } else if (n == 0x08) {
            let x: i64 = R[0];
            let r: i64 = 0;
            for (let bit: int = 15; bit >= 0; bit = bit - 1) {
                let cand: i64 = r | ((1 as i64) << bit);
                if (cand * cand <= x) { r = cand; }
            }
            R[0] = r;
        } else if (n == 0x0B) {
            this.cpuSet(false);
        } else if (n == 0x0C) {
            this.cpuSet(true);
        } else if (n == 0x0D) {
            R[0] = 3131961471;
        } else if (n == 0x11) {
            this.lzDecode(R[0], R[1], false);
        } else if (n == 0x12) {
            this.lzDecode(R[0], R[1], true);
        } else if (n == 0x14) {
            this.rlDecode(R[0], R[1], false);
        } else if (n == 0x15) {
            this.rlDecode(R[0], R[1], true);
        } else {
            this.swiMiss = this.swiMiss + 1;
            this.lastSwi = n;
        }
    }

    biosRamReset(flags: int) => void {
        if ((flags & 1) != 0) { for (let i: int = 0; i < 262144; i = i + 1) { GEWRAM[i] = 0 as u8; } }
        if ((flags & 2) != 0) { for (let i: int = 0; i < 32256; i = i + 1) { GIWRAM[i] = 0 as u8; } }
        if ((flags & 4) != 0) { for (let i: int = 0; i < 1024; i = i + 1) { GPAL[i] = 0 as u8; } }
        if ((flags & 8) != 0) { for (let i: int = 0; i < 98304; i = i + 1) { GVRAM[i] = 0 as u8; } }
        if ((flags & 16) != 0) { for (let i: int = 0; i < 1024; i = i + 1) { GOAM[i] = 0 as u8; } }
    }

    intrWait(discard: int, flags: int) => void {
        GIO[0x208] = 1 as u8;
        let lo: int = GIWRAM[0x7FF8];
        let hi: int = GIWRAM[0x7FF9];
        let cur: int = lo | (hi << 8);
        if (discard != 0) {
            cur = cur & (~flags);
            GIWRAM[0x7FF8] = (cur & 255) as u8;
            GIWRAM[0x7FF9] = ((cur >> 8) & 255) as u8;
        }
        this.waiting = true;
        this.waitFlags = flags;
    }

    // true (and the wait is over) once the IRQ handler has set one of the awaited flags
    waitDone() => bool {
        let lo: int = GIWRAM[0x7FF8];
        let hi: int = GIWRAM[0x7FF9];
        let cur: int = lo | (hi << 8);
        let done: bool = false;
        if ((cur & this.waitFlags) != 0) {
            cur = cur & (~this.waitFlags);
            GIWRAM[0x7FF8] = (cur & 255) as u8;
            GIWRAM[0x7FF9] = ((cur >> 8) & 255) as u8;
            this.waiting = false;
            done = true;
        }
        return done;
    }

    biosDiv(numr: i64, denr: i64) => void {
        let num: i64 = s32(numr);
        let den: i64 = s32(denr);
        if (den == 0) {
            if (num < 0) { R[0] = M32; } else { R[0] = 1; }
            R[1] = num & M32;
            R[3] = 1;
        } else {
            let q: i64 = num / den;
            let rem: i64 = num - (q * den);
            let aq: i64 = q;
            if (aq < 0) { aq = 0 - aq; }
            R[0] = q & M32;
            R[1] = rem & M32;
            R[3] = aq & M32;
        }
    }

    cpuSet(fast: bool) => void {
        let src: i64 = R[0];
        let dst: i64 = R[1];
        let ctl: i64 = R[2];
        let count: int = (ctl & 2097151) as int;
        let fill: bool = ((ctl >> 24) & 1) != 0;
        let word: bool = ((ctl >> 26) & 1) != 0;
        if (fast) {
            word = true;
            count = (count + 7) & (~7);
        }
        if (word) {
            let fv: i64 = this.get32(src);
            for (let i: int = 0; i < count; i = i + 1) {
                let v: i64 = fv;
                if (!fill) { v = this.get32(src + (i * 4)); }
                this.put16(dst + (i * 4), (v & 65535) as int);
                this.put16(dst + (i * 4) + 2, ((v >> 16) & 65535) as int);
            }
        } else {
            let fv: int = this.get16(src);
            for (let i: int = 0; i < count; i = i + 1) {
                let v: int = fv;
                if (!fill) { v = this.get16(src + (i * 2)); }
                this.put16(dst + (i * 2), v);
            }
        }
        this.cyc = this.cyc + (count * 2);
    }

    // GTMP -> memory: bytes for WRAM targets, halfwords for VRAM targets
    copyOut(dst: i64, size: int, half: bool) => void {
        if (half) {
            let i: int = 0;
            while (i < size) {
                let lo: int = GTMP[i];
                let hi: int = GTMP[i + 1];
                this.put16(dst + i, lo | (hi << 8));
                i = i + 2;
            }
        } else {
            for (let i: int = 0; i < size; i = i + 1) {
                let b: int = GTMP[i];
                this.put8(dst + i, b);
            }
        }
        this.cyc = this.cyc + (size * 2);
    }

    lzDecode(src: i64, dst: i64, half: bool) => void {
        let hdr: i64 = this.get32(src);
        let size: int = (hdr >> 8) as int;
        if (size > 262140) { size = 262140; }
        let sp: i64 = src + 4;
        let n: int = 0;
        while (n < size) {
            let flags: int = this.get8(sp);
            sp = sp + 1;
            for (let k: int = 0; k < 8; k = k + 1) {
                if (n < size) {
                    if ((flags & (128 >> k)) != 0) {
                        let b0: int = this.get8(sp);
                        let b1: int = this.get8(sp + 1);
                        sp = sp + 2;
                        let len: int = (b0 >> 4) + 3;
                        let disp: int = (((b0 & 15) << 8) | b1) + 1;
                        for (let j: int = 0; j < len; j = j + 1) {
                            if (n < size) {
                                let v: int = 0;
                                if (n - disp >= 0) { v = GTMP[n - disp]; }
                                GTMP[n] = v as u8;
                                n = n + 1;
                            }
                        }
                    } else {
                        let v: int = this.get8(sp);
                        sp = sp + 1;
                        GTMP[n] = v as u8;
                        n = n + 1;
                    }
                }
            }
        }
        this.copyOut(dst, size, half);
    }

    rlDecode(src: i64, dst: i64, half: bool) => void {
        let hdr: i64 = this.get32(src);
        let size: int = (hdr >> 8) as int;
        if (size > 262140) { size = 262140; }
        let sp: i64 = src + 4;
        let n: int = 0;
        while (n < size) {
            let f: int = this.get8(sp);
            sp = sp + 1;
            if ((f & 128) != 0) {
                let len: int = (f & 127) + 3;
                let v: int = this.get8(sp);
                sp = sp + 1;
                for (let j: int = 0; j < len; j = j + 1) {
                    if (n < size) { GTMP[n] = v as u8; n = n + 1; }
                }
            } else {
                let len: int = (f & 127) + 1;
                for (let j: int = 0; j < len; j = j + 1) {
                    let v: int = this.get8(sp);
                    sp = sp + 1;
                    if (n < size) { GTMP[n] = v as u8; n = n + 1; }
                }
            }
        }
        this.copyOut(dst, size, half);
    }

    // ---------------------------------------------------------------- interrupts and the frame loop
    enterIrq() => void {
        let old: i64 = this.getCpsr();
        let lr: i64 = (R[15] + 4) & M32;
        this.switchMode(0x12);
        BSPSR[2] = old;
        R[14] = lr;
        this.ctl = (this.ctl & 0xDF) | 0x80;
        R[15] = 0x18;
    }

    step() => int {
        this.cyc = 0;
        let ie0: int = GIO[0x200];
        let ie1: int = GIO[0x201];
        let ime: int = GIO[0x208];
        let pend: int = (ie0 | (ie1 << 8)) & this.iff & 0x3FFF;
        if (pend != 0) { this.halted = false; }
        if (pend != 0 && (ime & 1) == 1 && (this.ctl & 0x80) == 0) {
            this.enterIrq();
            this.cyc = this.cyc + 3;
        } else if (this.halted) {
            this.cyc = 8;
        } else if (this.waiting && !this.waitDone()) {
            this.cyc = 8;
        } else {
            let pc: i64 = R[15];
            if ((this.ctl & 0x20) != 0) {
                let op: int = this.rd16(pc);
                R[15] = (pc + 2) & M32;
                this.execThumb(op);
            } else {
                let op: i64 = this.rd32(pc);
                R[15] = (pc + 4) & M32;
                this.execArm(op);
            }
        }
        this.ppuAdvance(this.cyc);
        return this.cyc;
    }

    runFrame(budget: int) => int {
        let spent: int = 0;
        while (spent < budget) { spent = spent + this.step(); }
        return spent;
    }

    // ---------------------------------------------------------------- LCD
    pal(i: int) => int {
        let lo: int = GPAL[i * 2];
        let hi: int = GPAL[i * 2 + 1];
        return lo | (hi << 8);
    }

    ppuAdvance(c: int) => void {
        this.lineCyc = this.lineCyc + c;
        if (!this.hbl && this.lineCyc >= 960) {
            this.hbl = true;
            let ds: int = GIO[4];
            GIO[4] = (ds | 2) as u8;
            if (this.ly < 160) {
                this.renderLine(this.ly);
                if ((ds & 16) != 0) { this.iff = this.iff | 2; }
            }
        }
        if (this.lineCyc >= 1232) {
            this.lineCyc = this.lineCyc - 1232;
            this.hbl = false;
            this.ly = this.ly + 1;
            if (this.ly >= 228) { this.ly = 0; }
            GIO[6] = this.ly as u8;
            let ds2: int = GIO[4];
            ds2 = ds2 & 0xFD;
            if (this.ly == 160) {
                ds2 = ds2 | 1;
                if ((ds2 & 8) != 0) { this.iff = this.iff | 1; }
                this.frameDone = true;
            }
            if (this.ly == 0) { ds2 = ds2 & 0xFE; }
            let lyc: int = GIO[5];
            if (this.ly == lyc) {
                if ((ds2 & 4) == 0 && (ds2 & 32) != 0) { this.iff = this.iff | 4; }
                ds2 = ds2 | 4;
            } else {
                ds2 = ds2 & 0xFB;
            }
            GIO[4] = ds2 as u8;
        }
    }

    renderLine(y: int) => void {
        let d0: int = GIO[0];
        let d1: int = GIO[1];
        let dc: int = d0 | (d1 << 8);
        let mode: int = dc & 7;
        let bd: int = this.pal(0);
        if ((dc & 0x80) != 0) {
            for (let x: int = 0; x < 240; x = x + 1) { LB[x] = 0x7FFF; }
        } else {
            for (let x: int = 0; x < 240; x = x + 1) { LB[x] = bd; }
            if (mode == 0 || mode == 1) {
                let nbg: int = 4;
                if (mode == 1) { nbg = 2; }
                for (let p: int = 3; p >= 0; p = p - 1) {
                    for (let b: int = nbg - 1; b >= 0; b = b - 1) {
                        let cn: int = GIO[8 + 2 * b];
                        if (((dc >> (8 + b)) & 1) != 0 && (cn & 3) == p) { this.textBg(b, y); }
                    }
                }
            } else if (mode == 3) {
                if ((dc & 0x400) != 0) {
                    for (let x: int = 0; x < 240; x = x + 1) {
                        let o: int = y * 480 + x * 2;
                        let lo: int = GVRAM[o];
                        let hi: int = GVRAM[o + 1];
                        LB[x] = lo | (hi << 8);
                    }
                }
            } else if (mode == 4) {
                if ((dc & 0x400) != 0) {
                    let base: int = 0;
                    if ((dc & 0x10) != 0) { base = 40960; }
                    for (let x: int = 0; x < 240; x = x + 1) {
                        let idx: int = GVRAM[base + y * 240 + x];
                        if (idx != 0) { LB[x] = this.pal(idx); }
                    }
                }
            } else if (mode == 5) {
                if ((dc & 0x400) != 0 && y < 128) {
                    let base: int = 0;
                    if ((dc & 0x10) != 0) { base = 40960; }
                    for (let x: int = 0; x < 160; x = x + 1) {
                        let o: int = base + (y * 160 + x) * 2;
                        let lo: int = GVRAM[o];
                        let hi: int = GVRAM[o + 1];
                        LB[x] = lo | (hi << 8);
                    }
                }
            }
        }
        let fo: int = y * 960;
        for (let x: int = 0; x < 240; x = x + 1) {
            let c: int = LB[x];
            let r5: int = c & 31;
            let g5: int = (c >> 5) & 31;
            let b5: int = (c >> 10) & 31;
            let q: int = fo + x * 4;
            GFB[q] = ((r5 << 3) | (r5 >> 2)) as u8;
            GFB[q + 1] = ((g5 << 3) | (g5 >> 2)) as u8;
            GFB[q + 2] = ((b5 << 3) | (b5 >> 2)) as u8;
            GFB[q + 3] = 255 as u8;
        }
    }

    // One text-mode background layer for scanline y, drawn over LB (colour index 0 is transparent).
    textBg(b: int, y: int) => void {
        let c0: int = GIO[8 + 2 * b];
        let c1: int = GIO[9 + 2 * b];
        let cnt: int = c0 | (c1 << 8);
        let charBase: int = ((cnt >> 2) & 3) * 16384;
        let c8: bool = ((cnt >> 7) & 1) != 0;
        let sbase: int = ((cnt >> 8) & 31) * 2048;
        let size: int = (cnt >> 14) & 3;
        let h0: int = GIO[16 + 4 * b];
        let h1: int = GIO[17 + 4 * b];
        let v0: int = GIO[18 + 4 * b];
        let v1: int = GIO[19 + 4 * b];
        let hofs: int = (h0 | (h1 << 8)) & 511;
        let vofs: int = (v0 | (v1 << 8)) & 511;
        let wide: bool = (size & 1) != 0;
        let tall: bool = (size & 2) != 0;
        let wmask: int = 255;
        if (wide) { wmask = 511; }
        let hmask: int = 255;
        if (tall) { hmask = 511; }
        let my: int = (y + vofs) & hmask;
        for (let x: int = 0; x < 240; x = x + 1) {
            let mx: int = (x + hofs) & wmask;
            let blk: int = 0;
            if (wide && mx >= 256) { blk = 1; }
            if (tall && my >= 256) {
                if (wide) { blk = blk + 2; } else { blk = blk + 1; }
            }
            let tx: int = (mx & 255) >> 3;
            let ty: int = (my & 255) >> 3;
            let eo: int = sbase + blk * 2048 + (ty * 32 + tx) * 2;
            let e0: int = GVRAM[eo];
            let e1: int = GVRAM[eo + 1];
            let e: int = e0 | (e1 << 8);
            let tile: int = e & 1023;
            let px: int = mx & 7;
            let py: int = my & 7;
            if (((e >> 10) & 1) != 0) { px = 7 - px; }
            if (((e >> 11) & 1) != 0) { py = 7 - py; }
            let idx: int = 0;
            let col: int = 0;
            if (c8) {
                let ad: int = charBase + tile * 64 + py * 8 + px;
                if (ad < 98304) { idx = GVRAM[ad]; }
                if (idx != 0) { col = this.pal(idx); }
            } else {
                let ad: int = charBase + tile * 32 + py * 4 + (px >> 1);
                let by: int = 0;
                if (ad < 98304) { by = GVRAM[ad]; }
                if ((px & 1) != 0) { idx = by >> 4; } else { idx = by & 15; }
                if (idx != 0) { col = this.pal(((e >> 12) & 15) * 16 + idx); }
            }
            if (idx != 0) { LB[x] = col; }
        }
    }
}
