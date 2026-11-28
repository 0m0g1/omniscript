// docs/examples/gb/gb.os
/// Game Boy (DMG) core: CPU, bus, PPU, timers, interrupts, mappers (none, MBC1, MBC2, MBC3 + RTC, MBC5),
/// four-channel APU with 44.1 kHz stereo output, serial capture, Gameboy Doctor trace.
/// The core never prints. Registers are plain int fields, always kept masked (8 or 16 bit).
module Str = import "standard/1/String.os";

const FZ: int = 0x80;
const FN: int = 0x40;
const FH: int = 0x20;
const FC: int = 0x10;

let ROM: u8[2097152];
let CRAM: u8[32768];
let VRAM: u8[8192];
let WRAM: u8[8192];
let OAM: u8[160];
let HRAM: u8[128];
let IORAM: u8[128];
let FB: u8[23040];
let SERIAL: char[4096];
let TITLE: char[17];
let BGI: u8[160];
let SPI: u8[16];
let SPX: u8[16];
let ABUF: i16[8192];   // APU output: up to 4096 stereo frames per drain
let AR: int[64];       // APU registers FF10..FF26, indexed by IO offset
let RTCL: int[8];      // MBC3 live clock: S M H DL DH
let RTCT: int[8];      // MBC3 latched clock
let HEXD: char* = "0123456789ABCDEF";

struct GameBoy {
    a: int = 0;
    f: int = 0;
    b: int = 0;
    c: int = 0;
    d: int = 0;
    e: int = 0;
    h: int = 0;
    l: int = 0;
    sp: int = 0;
    pc: int = 0;
    ime: bool = false;
    imeCount: int = 0;
    halted: bool = false;
    haltBug: bool = false;
    ie: int = 0;
    iff: int = 0;
    ly: int = 0;
    dots: int = 0;
    lineDrawn: bool = false;
    statLine: bool = false;
    wl: int = 0;
    wyHit: bool = false;
    frameDone: bool = false;
    div16: int = 0;
    tima: int = 0;
    tma: int = 0;
    tac: int = 0;
    keys: int = 0;
    doctor: bool = false;
    mbc: int = 0;
    romBanks: int = 2;
    ramBytes: int = 0;
    battery: bool = false;
    ramEn: bool = false;
    bankLo: int = 1;
    bank2: int = 0;
    mode: int = 0;
    serN: int = 0;
    rtc: bool = false;
    rtcDirty: bool = false;
    ramSel: int = 0;
    rtcLatch: int = 0;
    apuOn: bool = false;
    fsCnt: int = 0;
    fsStep: int = 0;
    sAcc: int = 0;
    sumL: int = 0;
    sumR: int = 0;
    sumN: int = 0;
    hpL: int = 0;
    hpR: int = 0;
    aCnt: int = 0;
    c1On: bool = false;
    c1Len: int = 0;
    c1Tim: int = 0;
    c1Pos: int = 0;
    c1Vol: int = 0;
    c1Env: int = 0;
    sSh: int = 0;
    sTim: int = 0;
    sEn: bool = false;
    c2On: bool = false;
    c2Len: int = 0;
    c2Tim: int = 0;
    c2Pos: int = 0;
    c2Vol: int = 0;
    c2Env: int = 0;
    c3On: bool = false;
    c3Len: int = 0;
    c3Tim: int = 0;
    c3Pos: int = 0;
    c4On: bool = false;
    c4Len: int = 0;
    c4Tim: int = 0;
    c4Vol: int = 0;
    c4Env: int = 0;
    c4Lfsr: int = 0;

    // ---------------------------------------------------------------- setup
    reset() => void {
        this.a = 0x01;
        this.f = 0xB0;
        this.b = 0x00;
        this.c = 0x13;
        this.d = 0x00;
        this.e = 0xD8;
        this.h = 0x01;
        this.l = 0x4D;
        this.sp = 0xFFFE;
        this.pc = 0x0100;
        this.ime = false;
        this.imeCount = 0;
        this.halted = false;
        this.haltBug = false;
        this.ie = 0;
        this.iff = 0x01;
        this.ly = 0;
        this.dots = 0;
        this.lineDrawn = false;
        this.statLine = false;
        this.wl = 0;
        this.wyHit = false;
        this.frameDone = false;
        this.div16 = 0;
        this.tima = 0;
        this.tma = 0;
        this.tac = 0;
        this.keys = 0;
        this.ramEn = false;
        this.bankLo = 1;
        this.bank2 = 0;
        this.mode = 0;
        this.serN = 0;
        for (let i: int = 0; i < 8192; i = i + 1) {
            VRAM[i] = 0 as u8;
            WRAM[i] = 0 as u8;
        }
        for (let i: int = 0; i < 160; i = i + 1) { OAM[i] = 0 as u8; }
        for (let i: int = 0; i < 128; i = i + 1) {
            HRAM[i] = 0 as u8;
            IORAM[i] = 0 as u8;
        }
        for (let i: int = 0; i < 32768; i = i + 1) { CRAM[i] = 0 as u8; }
        for (let i: int = 0; i < 23040; i = i + 1) { FB[i] = 0 as u8; }
        SERIAL[0] = 0 as char;
        IORAM[0x40] = 0x91 as u8;
        IORAM[0x47] = 0xFC as u8;
        IORAM[0x48] = 0xFF as u8;
        IORAM[0x49] = 0xFF as u8;
        this.ramSel = 0;
        this.rtcLatch = 0;
        if (this.mbc == 0 && this.ramBytes > 0) { this.ramEn = true; }
        this.apuReset();
        this.apuOn = true;
        AR[0x10] = 0x80;
        AR[0x11] = 0xBF;
        AR[0x12] = 0xF3;
        AR[0x13] = 0xFF;
        AR[0x14] = 0xBF;
        AR[0x24] = 0x77;
        AR[0x25] = 0xF3;
        AR[0x26] = 0x80;
    }

    loadRom(data: u8*, len: int) => bool {
        let ok: bool = true;
        if (len < 0x150 || len > 2097152) { ok = false; }
        if (ok) {
            let ty: int = data[0x147];
            let rc: int = data[0x148];
            let ra: int = data[0x149];
            let m: int = -1;
            let ram: bool = false;
            let bat: bool = false;
            let tim: bool = false;
            switch (ty) {
                case 0x00: m = 0; break;
                case 0x01: m = 1; break;
                case 0x02: m = 1; ram = true; break;
                case 0x03: m = 1; ram = true; bat = true; break;
                case 0x05: m = 2; break;
                case 0x06: m = 2; bat = true; break;
                case 0x08: m = 0; ram = true; break;
                case 0x09: m = 0; ram = true; bat = true; break;
                case 0x0F: m = 3; tim = true; bat = true; break;
                case 0x10: m = 3; tim = true; ram = true; bat = true; break;
                case 0x11: m = 3; break;
                case 0x12: m = 3; ram = true; break;
                case 0x13: m = 3; ram = true; bat = true; break;
                case 0x19: m = 5; break;
                case 0x1A: m = 5; ram = true; break;
                case 0x1B: m = 5; ram = true; bat = true; break;
                case 0x1C: m = 5; break;
                case 0x1D: m = 5; ram = true; break;
                case 0x1E: m = 5; ram = true; bat = true; break;
                default: break;
            }
            if (m < 0) { ok = false; }
            if (rc > 6) { ok = false; }
            if (ram && m != 2 && ra > 3) { ok = false; }
            if (ok) {
                let banks: int = 2 << rc;
                let declared: int = banks * 16384;
                if (len > declared) { ok = false; }
                if (ok) {
                    for (let i: int = 0; i < len; i = i + 1) { ROM[i] = data[i]; }
                    for (let i: int = len; i < declared; i = i + 1) { ROM[i] = 0xFF as u8; }
                    this.romBanks = banks;
                    this.mbc = m;
                    this.battery = bat;
                    this.rtc = tim;
                    this.rtcDirty = false;
                    this.ramBytes = 0;
                    if (m == 2) {
                        this.ramBytes = 512;
                    } else if (ram) {
                        if (ra == 1) { this.ramBytes = 2048; }
                        if (ra == 2) { this.ramBytes = 8192; }
                        if (ra == 3) { this.ramBytes = 32768; }
                    }
                    for (let i: int = 0; i < 8; i = i + 1) { RTCL[i] = 0; RTCT[i] = 0; }
                    let tn: int = 0;
                    let tdone: bool = false;
                    for (let i: int = 0; i < 16; i = i + 1) {
                        let ch: int = data[0x134 + i];
                        if (ch < 32 || ch > 126) { tdone = true; }
                        if (!tdone) { TITLE[tn] = ch as char; tn = tn + 1; }
                    }
                    TITLE[tn] = 0 as char;
                    this.reset();
                }
            }
        }
        return ok;
    }

    // ---------------------------------------------------------------- bus
    rd(addr: int) => int {
        let r: int = 0xFF;
        let ad: int = addr & 0xFFFF;
        if (ad < 0x4000) {
            let bank0: int = 0;
            if (this.mbc == 1 && this.mode == 1) { bank0 = (this.bank2 << 5) & (this.romBanks - 1); }
            r = ROM[bank0 * 16384 + ad];
        } else if (ad < 0x8000) {
            let bank1: int = this.bankLo & (this.romBanks - 1);
            if (this.mbc == 1) { bank1 = ((this.bank2 << 5) | this.bankLo) & (this.romBanks - 1); }
            r = ROM[bank1 * 16384 + (ad - 0x4000)];
        } else if (ad < 0xA000) {
            r = VRAM[ad - 0x8000];
        } else if (ad < 0xC000) {
            r = this.cartRamRd(ad);
        } else if (ad < 0xE000) {
            r = WRAM[ad - 0xC000];
        } else if (ad < 0xFE00) {
            r = WRAM[ad - 0xE000];
        } else if (ad < 0xFEA0) {
            r = OAM[ad - 0xFE00];
        } else if (ad < 0xFF00) {
            r = 0xFF;
        } else if (ad < 0xFF80) {
            r = this.rdIO(ad & 0x7F);
        } else if (ad < 0xFFFF) {
            r = HRAM[ad - 0xFF80];
        } else {
            r = this.ie;
        }
        return r;
    }

    rdIO(n: int) => int {
        let r: int = IORAM[n];
        let sel: int = 0;
        let lo: int = 0;
        switch (n) {
            case 0x00:
                sel = IORAM[0] & 0x30;
                lo = 0x0F;
                if ((sel & 0x10) == 0) { lo = lo & (~(this.keys & 0x0F)); }
                if ((sel & 0x20) == 0) { lo = lo & (~((this.keys >> 4) & 0x0F)); }
                r = 0xC0 | sel | (lo & 0x0F);
                break;
            case 0x04:
                r = (this.div16 >> 8) & 0xFF;
                break;
            case 0x05:
                r = this.tima;
                break;
            case 0x06:
                r = this.tma;
                break;
            case 0x07:
                r = this.tac | 0xF8;
                break;
            case 0x0F:
                r = this.iff | 0xE0;
                break;
            case 0x41:
                r = 0x80 | (IORAM[0x41] & 0x78);
                if ((IORAM[0x40] & 0x80) != 0) {
                    let lyc: int = IORAM[0x45];
                    if (this.ly == lyc) { r = r | 0x04; }
                    r = r | this.mode();
                }
                break;
            case 0x44:
                if (this.doctor) { r = 0x90; } else { r = this.ly; }
                break;
            default:
                if (n >= 0x10 && n <= 0x26) {
                    r = this.apuRead(n);
                } else if (n >= 0x27 && n <= 0x2F) {
                    r = 0xFF;
                }
                break;
        }
        return r;
    }

    wr(addr: int, val: int) => void {
        let ad: int = addr & 0xFFFF;
        let v: int = val & 0xFF;
        if (ad < 0x8000) {
            if (this.mbc != 0) { this.mbcWrite(ad, v); }
        } else if (ad < 0xA000) {
            VRAM[ad - 0x8000] = v as u8;
        } else if (ad < 0xC000) {
            this.cartRamWr(ad, v);
        } else if (ad < 0xE000) {
            WRAM[ad - 0xC000] = v as u8;
        } else if (ad < 0xFE00) {
            WRAM[ad - 0xE000] = v as u8;
        } else if (ad < 0xFEA0) {
            OAM[ad - 0xFE00] = v as u8;
        } else if (ad < 0xFF00) {
            // unusable
        } else if (ad < 0xFF80) {
            this.wrIO(ad & 0x7F, v);
        } else if (ad < 0xFFFF) {
            HRAM[ad - 0xFF80] = v as u8;
        } else {
            this.ie = v;
        }
    }

    mbcWrite(ad: int, v: int) => void {
        if (this.mbc == 1) {
            if (ad < 0x2000) {
                this.ramEn = ((v & 0x0F) == 0x0A);
            } else if (ad < 0x4000) {
                this.bankLo = v & 0x1F;
                if (this.bankLo == 0) { this.bankLo = 1; }
            } else if (ad < 0x6000) {
                this.bank2 = v & 0x03;
            } else {
                this.mode = v & 0x01;
            }
        } else if (this.mbc == 2) {
            if (ad < 0x4000) {
                if ((ad & 0x100) == 0) {
                    this.ramEn = ((v & 0x0F) == 0x0A);
                } else {
                    this.bankLo = v & 0x0F;
                    if (this.bankLo == 0) { this.bankLo = 1; }
                }
            }
        } else if (this.mbc == 3) {
            if (ad < 0x2000) {
                this.ramEn = ((v & 0x0F) == 0x0A);
            } else if (ad < 0x4000) {
                this.bankLo = v & 0x7F;
                if (this.bankLo == 0) { this.bankLo = 1; }
            } else if (ad < 0x6000) {
                this.ramSel = v & 0x0F;
            } else {
                if (this.rtcLatch == 0 && (v & 1) == 1) {
                    for (let i: int = 0; i < 5; i = i + 1) { RTCT[i] = RTCL[i]; }
                }
                this.rtcLatch = v & 1;
            }
        } else if (this.mbc == 5) {
            if (ad < 0x2000) {
                this.ramEn = ((v & 0x0F) == 0x0A);
            } else if (ad < 0x3000) {
                this.bankLo = (this.bankLo & 0x100) | v;
            } else if (ad < 0x4000) {
                this.bankLo = (this.bankLo & 0xFF) | ((v & 1) << 8);
            } else if (ad < 0x6000) {
                this.ramSel = v & 0x0F;
            }
        }
    }

    // ---------------------------------------------------------------- cartridge RAM and MBC3 clock
    cartRamRd(ad: int) => int {
        let r: int = 0xFF;
        if (this.mbc == 2) {
            if (this.ramEn) {
                let b: int = CRAM[(ad - 0xA000) & 0x1FF];
                r = 0xF0 | (b & 0x0F);
            }
        } else if (this.mbc == 3 && this.ramSel >= 8) {
            if (this.ramEn && this.rtc && this.ramSel <= 12) { r = RTCT[this.ramSel - 8]; }
        } else if (this.ramEn && this.ramBytes > 0) {
            let rb: int = 0;
            if (this.mbc == 1) {
                if (this.mode == 1) { rb = this.bank2; }
            } else if (this.mbc == 3 || this.mbc == 5) {
                rb = this.ramSel;
            }
            r = CRAM[(rb * 8192 + (ad - 0xA000)) % this.ramBytes];
        }
        return r;
    }

    cartRamWr(ad: int, v: int) => void {
        if (this.mbc == 2) {
            if (this.ramEn) { CRAM[(ad - 0xA000) & 0x1FF] = (v & 0x0F) as u8; }
        } else if (this.mbc == 3 && this.ramSel >= 8) {
            if (this.ramEn && this.rtc && this.ramSel <= 12) {
                let i: int = this.ramSel - 8;
                let mk: int = 0xFF;
                if (i < 2) { mk = 0x3F; }
                if (i == 2) { mk = 0x1F; }
                if (i == 4) { mk = 0xC1; }
                RTCL[i] = v & mk;
                this.rtcDirty = true;
            }
        } else if (this.ramEn && this.ramBytes > 0) {
            let rb: int = 0;
            if (this.mbc == 1) {
                if (this.mode == 1) { rb = this.bank2; }
            } else if (this.mbc == 3 || this.mbc == 5) {
                rb = this.ramSel;
            }
            CRAM[(rb * 8192 + (ad - 0xA000)) % this.ramBytes] = v as u8;
        }
    }

    // Advances the MBC3 clock by secs real seconds (ignored when the cart has no clock or it is halted).
    rtcAdvance(secs: int) => void {
        if (this.rtc && secs > 0 && (RTCL[4] & 0x40) == 0) {
            let s: int = secs;
            if (s > 40000000) { s = 40000000; }
            let tod: int = RTCL[0] + RTCL[1] * 60 + RTCL[2] * 3600 + s;
            let days: int = RTCL[3] | ((RTCL[4] & 1) << 8);
            days = days + tod / 86400;
            tod = tod % 86400;
            let carry: int = RTCL[4] & 0x80;
            if (days > 511) { days = days & 511; carry = 0x80; }
            RTCL[0] = tod % 60;
            RTCL[1] = (tod / 60) % 60;
            RTCL[2] = tod / 3600;
            RTCL[3] = days & 0xFF;
            RTCL[4] = (RTCL[4] & 0x40) | carry | ((days >> 8) & 1);
        }
    }

    // 16-byte clock record for the .sav file: live S M H DL DH, marker 0xA5, unix time of saving (4 bytes LE).
    rtcExport(out: u8*, now: int) => void {
        for (let i: int = 0; i < 16; i = i + 1) { out[i] = 0 as u8; }
        for (let i: int = 0; i < 5; i = i + 1) { out[i] = RTCL[i] as u8; }
        out[5] = 0xA5 as u8;
        out[8] = (now & 0xFF) as u8;
        out[9] = ((now >> 8) & 0xFF) as u8;
        out[10] = ((now >> 16) & 0xFF) as u8;
        out[11] = ((now >> 24) & 0xFF) as u8;
    }

    // Loads a record written by rtcExport and lets the clock run for the time that passed since.
    rtcImport(inp: u8*, now: int) => void {
        let mk: int = inp[5];
        if (mk == 0xA5) {
            for (let i: int = 0; i < 5; i = i + 1) {
                let t: int = inp[i];
                RTCL[i] = t;
                RTCT[i] = t;
            }
            let b0: int = inp[8];
            let b1: int = inp[9];
            let b2: int = inp[10];
            let b3: int = inp[11];
            let saved: int = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
            if (now > saved) { this.rtcAdvance(now - saved); }
        }
    }

    wrIO(n: int, v: int) => void {
        let src: int = 0;
        let old: bool = false;
        switch (n) {
            case 0x00:
                IORAM[0] = (v & 0x30) as u8;
                break;
            case 0x02:
                IORAM[2] = v as u8;
                if ((v & 0x81) == 0x81) {
                    if (this.serN < 4095) {
                        SERIAL[this.serN] = IORAM[1] as char;
                        SERIAL[this.serN + 1] = 0 as char;
                        this.serN = this.serN + 1;
                    }
                    IORAM[2] = (v & 0x7F) as u8;
                }
                break;
            case 0x04:
                old = this.tsig();
                this.div16 = 0;
                if (old && !this.tsig()) { this.incTima(); }
                break;
            case 0x05:
                this.tima = v;
                break;
            case 0x06:
                this.tma = v;
                break;
            case 0x07:
                old = this.tsig();
                this.tac = v & 7;
                if (old && !this.tsig()) { this.incTima(); }
                break;
            case 0x0F:
                this.iff = v & 0x1F;
                break;
            case 0x40:
                old = ((IORAM[0x40] & 0x80) != 0);
                IORAM[0x40] = v as u8;
                if (old && (v & 0x80) == 0) {
                    for (let i: int = 0; i < 23040; i = i + 1) { FB[i] = 0 as u8; }
                    this.ly = 0;
                    this.dots = 0;
                    this.lineDrawn = false;
                    this.statLine = false;
                    this.wl = 0;
                    this.wyHit = false;
                }
                if (!old && (v & 0x80) != 0) {
                    this.ly = 0;
                    this.dots = 0;
                    this.lineDrawn = false;
                    this.wl = 0;
                    this.wyHit = false;
                }
                break;
            case 0x44:
                break;
            case 0x46:
                IORAM[0x46] = v as u8;
                src = v << 8;
                for (let i: int = 0; i < 160; i = i + 1) { OAM[i] = this.rd(src + i) as u8; }
                break;
            default:
                if (n >= 0x10 && n <= 0x26) {
                    this.apuWrite(n, v);
                } else {
                    IORAM[n] = v as u8;
                }
                break;
        }
    }

    // ---------------------------------------------------------------- register helpers
    hl() => int { return (this.h << 8) | this.l; }

    fetch() => int {
        let v: int = this.rd(this.pc);
        if (this.haltBug) {
            this.haltBug = false;
        } else {
            this.pc = (this.pc + 1) & 0xFFFF;
        }
        return v;
    }

    fetch16() => int {
        let lo: int = this.fetch();
        let hi: int = this.fetch();
        return lo | (hi << 8);
    }

    push(v: int) => void {
        this.sp = (this.sp - 1) & 0xFFFF;
        this.wr(this.sp, v >> 8);
        this.sp = (this.sp - 1) & 0xFFFF;
        this.wr(this.sp, v & 0xFF);
    }

    pop() => int {
        let lo: int = this.rd(this.sp);
        this.sp = (this.sp + 1) & 0xFFFF;
        let hi: int = this.rd(this.sp);
        this.sp = (this.sp + 1) & 0xFFFF;
        return lo | (hi << 8);
    }

    getR(i: int) => int {
        let r: int = 0;
        switch (i) {
            case 0: r = this.b; break;
            case 1: r = this.c; break;
            case 2: r = this.d; break;
            case 3: r = this.e; break;
            case 4: r = this.h; break;
            case 5: r = this.l; break;
            case 6: r = this.rd(this.hl()); break;
            default: r = this.a; break;
        }
        return r;
    }

    setR(i: int, val: int) => void {
        let v: int = val & 0xFF;
        switch (i) {
            case 0: this.b = v; break;
            case 1: this.c = v; break;
            case 2: this.d = v; break;
            case 3: this.e = v; break;
            case 4: this.h = v; break;
            case 5: this.l = v; break;
            case 6: this.wr(this.hl(), v); break;
            default: this.a = v; break;
        }
    }

    rp(i: int) => int {
        let r: int = 0;
        switch (i) {
            case 0: r = (this.b << 8) | this.c; break;
            case 1: r = (this.d << 8) | this.e; break;
            case 2: r = (this.h << 8) | this.l; break;
            default: r = this.sp; break;
        }
        return r;
    }

    setRp(i: int, val: int) => void {
        let v: int = val & 0xFFFF;
        switch (i) {
            case 0: this.b = v >> 8; this.c = v & 0xFF; break;
            case 1: this.d = v >> 8; this.e = v & 0xFF; break;
            case 2: this.h = v >> 8; this.l = v & 0xFF; break;
            default: this.sp = v; break;
        }
    }

    rp2(i: int) => int {
        let r: int = 0;
        if (i == 3) { r = (this.a << 8) | this.f; } else { r = this.rp(i); }
        return r;
    }

    setRp2(i: int, val: int) => void {
        if (i == 3) {
            this.a = (val >> 8) & 0xFF;
            this.f = val & 0xF0;
        } else {
            this.setRp(i, val);
        }
    }

    cond(cc: int) => bool {
        let r: bool = false;
        switch (cc) {
            case 0: r = ((this.f & FZ) == 0); break;
            case 1: r = ((this.f & FZ) != 0); break;
            case 2: r = ((this.f & FC) == 0); break;
            default: r = ((this.f & FC) != 0); break;
        }
        return r;
    }

    // ---------------------------------------------------------------- ALU
    alu(op: int, v: int) => void {
        let x: int = this.a;
        let r: int = 0;
        let cy: int = 0;
        let fl: int = 0;
        if (op == 1 || op == 3) {
            if ((this.f & FC) != 0) { cy = 1; }
        }
        if (op == 0 || op == 1) {
            r = x + v + cy;
            if (((x & 0x0F) + (v & 0x0F) + cy) > 0x0F) { fl = fl | FH; }
            if (r > 0xFF) { fl = fl | FC; }
            r = r & 0xFF;
        } else if (op == 2 || op == 3 || op == 7) {
            r = x - v - cy;
            fl = FN;
            if (((x & 0x0F) - (v & 0x0F) - cy) < 0) { fl = fl | FH; }
            if (r < 0) { fl = fl | FC; }
            r = r & 0xFF;
        } else if (op == 4) {
            r = x & v;
            fl = FH;
        } else if (op == 5) {
            r = x ^ v;
        } else {
            r = x | v;
        }
        if (r == 0) { fl = fl | FZ; }
        this.f = fl;
        if (op != 7) { this.a = r; }
    }

    addHL(v: int) => void {
        let hl: int = this.hl();
        let r: int = hl + v;
        let fl: int = this.f & FZ;
        if (((hl & 0x0FFF) + (v & 0x0FFF)) > 0x0FFF) { fl = fl | FH; }
        if (r > 0xFFFF) { fl = fl | FC; }
        this.f = fl;
        this.setRp(2, r & 0xFFFF);
    }

    // SP + signed byte; sets flags like ADD SP,e. Returns the 16-bit result.
    spPlus(raw: int) => int {
        let se: int = raw;
        if (se > 127) { se = se - 256; }
        let fl: int = 0;
        if (((this.sp & 0x0F) + (raw & 0x0F)) > 0x0F) { fl = fl | FH; }
        if (((this.sp & 0xFF) + raw) > 0xFF) { fl = fl | FC; }
        this.f = fl;
        return (this.sp + se) & 0xFFFF;
    }

    daa() => void {
        let x: int = this.a;
        let adj: int = 0;
        let cf: bool = ((this.f & FC) != 0);
        if ((this.f & FN) == 0) {
            if ((this.f & FH) != 0 || (x & 0x0F) > 9) { adj = 6; }
            if (cf || x > 0x99) {
                adj = adj | 0x60;
                cf = true;
            }
            x = (x + adj) & 0xFF;
        } else {
            if ((this.f & FH) != 0) { adj = 6; }
            if (cf) { adj = adj | 0x60; }
            x = (x - adj) & 0xFF;
        }
        let fl: int = this.f & FN;
        if (x == 0) { fl = fl | FZ; }
        if (cf) { fl = fl | FC; }
        this.f = fl;
        this.a = x;
    }

    // rotate/shift group for CB (and y of 0..7); returns the result, sets flags.
    rot(y: int, v: int) => int {
        let r: int = 0;
        let co: int = 0;
        let oc: int = 0;
        if ((this.f & FC) != 0) { oc = 1; }
        switch (y) {
            case 0: co = v >> 7; r = ((v << 1) | co) & 0xFF; break;
            case 1: co = v & 1; r = (v >> 1) | (co << 7); break;
            case 2: co = v >> 7; r = ((v << 1) | oc) & 0xFF; break;
            case 3: co = v & 1; r = (v >> 1) | (oc << 7); break;
            case 4: co = v >> 7; r = (v << 1) & 0xFF; break;
            case 5: co = v & 1; r = (v >> 1) | (v & 0x80); break;
            case 6: co = 0; r = ((v << 4) | (v >> 4)) & 0xFF; break;
            default: co = v & 1; r = v >> 1; break;
        }
        let fl: int = 0;
        if (r == 0) { fl = fl | FZ; }
        if (co != 0) { fl = fl | FC; }
        this.f = fl;
        return r;
    }

    execCB() => int {
        let op: int = this.fetch();
        let x: int = op >> 6;
        let y: int = (op >> 3) & 7;
        let z: int = op & 7;
        let v: int = this.getR(z);
        let t: int = 8;
        let r: int = 0;
        if (x == 0) {
            r = this.rot(y, v);
            this.setR(z, r);
            if (z == 6) { t = 16; }
        } else if (x == 1) {
            let fl: int = (this.f & FC) | FH;
            if ((v & (1 << y)) == 0) { fl = fl | FZ; }
            this.f = fl;
            if (z == 6) { t = 12; }
        } else if (x == 2) {
            this.setR(z, v & (0xFF ^ (1 << y)));
            if (z == 6) { t = 16; }
        } else {
            this.setR(z, v | (1 << y));
            if (z == 6) { t = 16; }
        }
        return t;
    }

    // ---------------------------------------------------------------- instruction decode
    exec07(y: int) => void {
        let co: int = 0;
        let oc: int = 0;
        let fl: int = 0;
        if ((this.f & FC) != 0) { oc = 1; }
        switch (y) {
            case 0:
                co = this.a >> 7;
                this.a = ((this.a << 1) | co) & 0xFF;
                if (co != 0) { fl = FC; }
                this.f = fl;
                break;
            case 1:
                co = this.a & 1;
                this.a = (this.a >> 1) | (co << 7);
                if (co != 0) { fl = FC; }
                this.f = fl;
                break;
            case 2:
                co = this.a >> 7;
                this.a = ((this.a << 1) | oc) & 0xFF;
                if (co != 0) { fl = FC; }
                this.f = fl;
                break;
            case 3:
                co = this.a & 1;
                this.a = (this.a >> 1) | (oc << 7);
                if (co != 0) { fl = FC; }
                this.f = fl;
                break;
            case 4:
                this.daa();
                break;
            case 5:
                this.a = this.a ^ 0xFF;
                this.f = this.f | FN | FH;
                break;
            case 6:
                this.f = (this.f & FZ) | FC;
                break;
            default:
                this.f = (this.f & FZ) | ((this.f & FC) ^ FC);
                break;
        }
    }

    exec0(y: int, z: int, p: int, q: int) => int {
        let t: int = 4;
        let nn: int = 0;
        let se: int = 0;
        let v: int = 0;
        let r: int = 0;
        let fl: int = 0;
        switch (z) {
            case 0:
                if (y == 0) {
                    t = 4;
                } else if (y == 1) {
                    nn = this.fetch16();
                    this.wr(nn, this.sp & 0xFF);
                    this.wr(nn + 1, this.sp >> 8);
                    t = 20;
                } else if (y == 2) {
                    this.fetch();
                    t = 4;
                } else if (y == 3) {
                    se = this.fetch();
                    if (se > 127) { se = se - 256; }
                    this.pc = (this.pc + se) & 0xFFFF;
                    t = 12;
                } else {
                    se = this.fetch();
                    if (se > 127) { se = se - 256; }
                    t = 8;
                    if (this.cond(y - 4)) {
                        this.pc = (this.pc + se) & 0xFFFF;
                        t = 12;
                    }
                }
                break;
            case 1:
                if (q == 0) {
                    nn = this.fetch16();
                    this.setRp(p, nn);
                    t = 12;
                } else {
                    this.addHL(this.rp(p));
                    t = 8;
                }
                break;
            case 2:
                t = 8;
                if (q == 0) {
                    if (p == 0) {
                        this.wr(this.rp(0), this.a);
                    } else if (p == 1) {
                        this.wr(this.rp(1), this.a);
                    } else if (p == 2) {
                        this.wr(this.hl(), this.a);
                        this.setRp(2, (this.hl() + 1) & 0xFFFF);
                    } else {
                        this.wr(this.hl(), this.a);
                        this.setRp(2, (this.hl() - 1) & 0xFFFF);
                    }
                } else {
                    if (p == 0) {
                        this.a = this.rd(this.rp(0));
                    } else if (p == 1) {
                        this.a = this.rd(this.rp(1));
                    } else if (p == 2) {
                        this.a = this.rd(this.hl());
                        this.setRp(2, (this.hl() + 1) & 0xFFFF);
                    } else {
                        this.a = this.rd(this.hl());
                        this.setRp(2, (this.hl() - 1) & 0xFFFF);
                    }
                }
                break;
            case 3:
                t = 8;
                if (q == 0) {
                    this.setRp(p, (this.rp(p) + 1) & 0xFFFF);
                } else {
                    this.setRp(p, (this.rp(p) - 1) & 0xFFFF);
                }
                break;
            case 4:
                v = this.getR(y);
                r = (v + 1) & 0xFF;
                fl = this.f & FC;
                if (r == 0) { fl = fl | FZ; }
                if ((v & 0x0F) == 0x0F) { fl = fl | FH; }
                this.f = fl;
                this.setR(y, r);
                t = 4;
                if (y == 6) { t = 12; }
                break;
            case 5:
                v = this.getR(y);
                r = (v - 1) & 0xFF;
                fl = (this.f & FC) | FN;
                if (r == 0) { fl = fl | FZ; }
                if ((v & 0x0F) == 0) { fl = fl | FH; }
                this.f = fl;
                this.setR(y, r);
                t = 4;
                if (y == 6) { t = 12; }
                break;
            case 6:
                v = this.fetch();
                this.setR(y, v);
                t = 8;
                if (y == 6) { t = 12; }
                break;
            default:
                this.exec07(y);
                t = 4;
                break;
        }
        return t;
    }

    exec3(y: int, z: int, p: int, q: int) => int {
        let t: int = 4;
        let nn: int = 0;
        let v: int = 0;
        let raw: int = 0;
        switch (z) {
            case 0:
                if (y < 4) {
                    t = 8;
                    if (this.cond(y)) {
                        this.pc = this.pop();
                        t = 20;
                    }
                } else if (y == 4) {
                    v = this.fetch();
                    this.wr(0xFF00 + v, this.a);
                    t = 12;
                } else if (y == 5) {
                    raw = this.fetch();
                    this.sp = this.spPlus(raw);
                    t = 16;
                } else if (y == 6) {
                    v = this.fetch();
                    this.a = this.rd(0xFF00 + v);
                    t = 12;
                } else {
                    raw = this.fetch();
                    this.setRp(2, this.spPlus(raw));
                    t = 12;
                }
                break;
            case 1:
                if (q == 0) {
                    nn = this.pop();
                    this.setRp2(p, nn);
                    t = 12;
                } else if (p == 0) {
                    this.pc = this.pop();
                    t = 16;
                } else if (p == 1) {
                    this.pc = this.pop();
                    this.ime = true;
                    this.imeCount = 0;
                    t = 16;
                } else if (p == 2) {
                    this.pc = this.hl();
                    t = 4;
                } else {
                    this.sp = this.hl();
                    t = 8;
                }
                break;
            case 2:
                if (y < 4) {
                    nn = this.fetch16();
                    t = 12;
                    if (this.cond(y)) {
                        this.pc = nn;
                        t = 16;
                    }
                } else if (y == 4) {
                    this.wr(0xFF00 + this.c, this.a);
                    t = 8;
                } else if (y == 5) {
                    nn = this.fetch16();
                    this.wr(nn, this.a);
                    t = 16;
                } else if (y == 6) {
                    this.a = this.rd(0xFF00 + this.c);
                    t = 8;
                } else {
                    nn = this.fetch16();
                    this.a = this.rd(nn);
                    t = 16;
                }
                break;
            case 3:
                if (y == 0) {
                    nn = this.fetch16();
                    this.pc = nn;
                    t = 16;
                } else if (y == 1) {
                    t = this.execCB();
                } else if (y == 6) {
                    this.ime = false;
                    this.imeCount = 0;
                    t = 4;
                } else if (y == 7) {
                    this.imeCount = 2;
                    t = 4;
                } else {
                    t = 4;
                }
                break;
            case 4:
                if (y < 4) {
                    nn = this.fetch16();
                    t = 12;
                    if (this.cond(y)) {
                        this.push(this.pc);
                        this.pc = nn;
                        t = 24;
                    }
                } else {
                    t = 4;
                }
                break;
            case 5:
                if (q == 0) {
                    this.push(this.rp2(p));
                    t = 16;
                } else if (p == 0) {
                    nn = this.fetch16();
                    this.push(this.pc);
                    this.pc = nn;
                    t = 24;
                } else {
                    t = 4;
                }
                break;
            case 6:
                v = this.fetch();
                this.alu(y, v);
                t = 8;
                break;
            default:
                this.push(this.pc);
                this.pc = y * 8;
                t = 16;
                break;
        }
        return t;
    }

    execOne() => int {
        let op: int = this.fetch();
        let x: int = op >> 6;
        let y: int = (op >> 3) & 7;
        let z: int = op & 7;
        let t: int = 4;
        if (x == 0) {
            t = this.exec0(y, z, y >> 1, y & 1);
        } else if (x == 1) {
            if (op == 0x76) {
                let pend: int = this.ie & this.iff & 0x1F;
                if (!this.ime && pend != 0) {
                    this.haltBug = true;
                } else {
                    this.halted = true;
                }
                t = 4;
            } else {
                this.setR(y, this.getR(z));
                t = 4;
                if (y == 6 || z == 6) { t = 8; }
            }
        } else if (x == 2) {
            this.alu(y, this.getR(z));
            t = 4;
            if (z == 6) { t = 8; }
        } else {
            t = this.exec3(y, z, y >> 1, y & 1);
        }
        return t;
    }

    // ---------------------------------------------------------------- APU
    // Four channels, frame sequencer (512 Hz), box-filtered 44.1 kHz stereo output into ABUF.
    // Registers FF10..FF26 live in AR[]; wave RAM FF30..FF3F lives in IORAM.
    apuReset() => void {
        for (let i: int = 0; i < 64; i = i + 1) { AR[i] = 0; }
        this.apuOn = false;
        this.fsCnt = 0;
        this.fsStep = 0;
        this.sAcc = 0;
        this.sumL = 0;
        this.sumR = 0;
        this.sumN = 0;
        this.hpL = 0;
        this.hpR = 0;
        this.aCnt = 0;
        this.c1On = false; this.c1Len = 0; this.c1Tim = 0; this.c1Pos = 0; this.c1Vol = 0; this.c1Env = 0;
        this.sSh = 0; this.sTim = 0; this.sEn = false;
        this.c2On = false; this.c2Len = 0; this.c2Tim = 0; this.c2Pos = 0; this.c2Vol = 0; this.c2Env = 0;
        this.c3On = false; this.c3Len = 0; this.c3Tim = 0; this.c3Pos = 0;
        this.c4On = false; this.c4Len = 0; this.c4Tim = 0; this.c4Vol = 0; this.c4Env = 0; this.c4Lfsr = 0x7FFF;
    }

    apuRead(n: int) => int {
        let r: int = 0;
        if (n == 0x26) {
            r = 0x70;
            if (this.apuOn) { r = r | 0x80; }
            if (this.c1On) { r = r | 0x01; }
            if (this.c2On) { r = r | 0x02; }
            if (this.c3On) { r = r | 0x04; }
            if (this.c4On) { r = r | 0x08; }
        } else {
            let mk: int = 0;
            switch (n) {
                case 0x10: mk = 0x80; break;
                case 0x11: mk = 0x3F; break;
                case 0x13: mk = 0xFF; break;
                case 0x14: mk = 0xBF; break;
                case 0x15: mk = 0xFF; break;
                case 0x16: mk = 0x3F; break;
                case 0x18: mk = 0xFF; break;
                case 0x19: mk = 0xBF; break;
                case 0x1A: mk = 0x7F; break;
                case 0x1B: mk = 0xFF; break;
                case 0x1C: mk = 0x9F; break;
                case 0x1D: mk = 0xFF; break;
                case 0x1E: mk = 0xBF; break;
                case 0x1F: mk = 0xFF; break;
                case 0x20: mk = 0xFF; break;
                case 0x23: mk = 0xBF; break;
                default: break;
            }
            r = AR[n] | mk;
        }
        return r;
    }

    apuWrite(n: int, v: int) => void {
        if (n == 0x26) {
            let was: bool = this.apuOn;
            this.apuOn = ((v & 0x80) != 0);
            if (was && !this.apuOn) {
                for (let i: int = 0x10; i < 0x26; i = i + 1) { AR[i] = 0; }
                this.c1On = false;
                this.c2On = false;
                this.c3On = false;
                this.c4On = false;
            }
            if (!was && this.apuOn) {
                this.fsStep = 0;
                this.fsCnt = 0;
                this.c1Pos = 0;
                this.c2Pos = 0;
                this.c3Pos = 0;
            }
            AR[0x26] = v & 0x80;
        } else if (this.apuOn && n < 0x26) {
            AR[n] = v;
            switch (n) {
                case 0x11: this.c1Len = 64 - (v & 0x3F); break;
                case 0x12: if ((v & 0xF8) == 0) { this.c1On = false; } break;
                case 0x14: if ((v & 0x80) != 0) { this.trig1(); } break;
                case 0x16: this.c2Len = 64 - (v & 0x3F); break;
                case 0x17: if ((v & 0xF8) == 0) { this.c2On = false; } break;
                case 0x19: if ((v & 0x80) != 0) { this.trig2(); } break;
                case 0x1A: if ((v & 0x80) == 0) { this.c3On = false; } break;
                case 0x1B: this.c3Len = 256 - v; break;
                case 0x1E: if ((v & 0x80) != 0) { this.trig3(); } break;
                case 0x20: this.c4Len = 64 - (v & 0x3F); break;
                case 0x21: if ((v & 0xF8) == 0) { this.c4On = false; } break;
                case 0x23: if ((v & 0x80) != 0) { this.trig4(); } break;
                default: break;
            }
        }
    }

    trig1() => void {
        this.c1On = ((AR[0x12] & 0xF8) != 0);
        if (this.c1Len == 0) { this.c1Len = 64; }
        let f: int = AR[0x13] | ((AR[0x14] & 7) << 8);
        this.c1Tim = (2048 - f) * 4;
        this.c1Env = AR[0x12] & 7;
        this.c1Vol = (AR[0x12] >> 4) & 15;
        this.sSh = f;
        let per: int = (AR[0x10] >> 4) & 7;
        let sh: int = AR[0x10] & 7;
        this.sTim = per;
        if (per == 0) { this.sTim = 8; }
        this.sEn = (per != 0 || sh != 0);
        if (sh != 0) { this.sweepCalc(); }
    }

    trig2() => void {
        this.c2On = ((AR[0x17] & 0xF8) != 0);
        if (this.c2Len == 0) { this.c2Len = 64; }
        let f: int = AR[0x18] | ((AR[0x19] & 7) << 8);
        this.c2Tim = (2048 - f) * 4;
        this.c2Env = AR[0x17] & 7;
        this.c2Vol = (AR[0x17] >> 4) & 15;
    }

    trig3() => void {
        this.c3On = ((AR[0x1A] & 0x80) != 0);
        if (this.c3Len == 0) { this.c3Len = 256; }
        let f: int = AR[0x1D] | ((AR[0x1E] & 7) << 8);
        this.c3Tim = (2048 - f) * 2 + 6;
        this.c3Pos = 0;
    }

    trig4() => void {
        this.c4On = ((AR[0x21] & 0xF8) != 0);
        if (this.c4Len == 0) { this.c4Len = 64; }
        this.c4Lfsr = 0x7FFF;
        this.c4Tim = this.noisePeriod();
        this.c4Env = AR[0x21] & 7;
        this.c4Vol = (AR[0x21] >> 4) & 15;
    }

    noisePeriod() => int {
        let d: int = AR[0x22] & 7;
        let base: int = 8;
        if (d != 0) { base = d * 16; }
        return base << ((AR[0x22] >> 4) & 15);
    }

    // Returns the new frequency; turns channel 1 off when it would overflow.
    sweepCalc() => int {
        let sh: int = AR[0x10] & 7;
        let d: int = this.sSh >> sh;
        let nf: int = this.sSh + d;
        if ((AR[0x10] & 8) != 0) { nf = this.sSh - d; }
        if (nf > 2047) { this.c1On = false; }
        return nf;
    }

    sweepClock() => void {
        if (this.c1On && this.sEn) {
            this.sTim = this.sTim - 1;
            if (this.sTim <= 0) {
                let per: int = (AR[0x10] >> 4) & 7;
                if (per == 0) {
                    this.sTim = 8;
                } else {
                    this.sTim = per;
                    let sh: int = AR[0x10] & 7;
                    let nf: int = this.sweepCalc();
                    if (nf <= 2047 && nf >= 0 && sh != 0) {
                        this.sSh = nf;
                        AR[0x13] = nf & 0xFF;
                        AR[0x14] = (AR[0x14] & 0xF8) | ((nf >> 8) & 7);
                        this.sweepCalc();
                    }
                }
            }
        }
    }

    lenClock() => void {
        if ((AR[0x14] & 0x40) != 0 && this.c1Len > 0) {
            this.c1Len = this.c1Len - 1;
            if (this.c1Len == 0) { this.c1On = false; }
        }
        if ((AR[0x19] & 0x40) != 0 && this.c2Len > 0) {
            this.c2Len = this.c2Len - 1;
            if (this.c2Len == 0) { this.c2On = false; }
        }
        if ((AR[0x1E] & 0x40) != 0 && this.c3Len > 0) {
            this.c3Len = this.c3Len - 1;
            if (this.c3Len == 0) { this.c3On = false; }
        }
        if ((AR[0x23] & 0x40) != 0 && this.c4Len > 0) {
            this.c4Len = this.c4Len - 1;
            if (this.c4Len == 0) { this.c4On = false; }
        }
    }

    envClock() => void {
        let p1: int = AR[0x12] & 7;
        if (this.c1On && p1 != 0) {
            this.c1Env = this.c1Env - 1;
            if (this.c1Env <= 0) {
                this.c1Env = p1;
                if ((AR[0x12] & 8) != 0) {
                    if (this.c1Vol < 15) { this.c1Vol = this.c1Vol + 1; }
                } else {
                    if (this.c1Vol > 0) { this.c1Vol = this.c1Vol - 1; }
                }
            }
        }
        let p2: int = AR[0x17] & 7;
        if (this.c2On && p2 != 0) {
            this.c2Env = this.c2Env - 1;
            if (this.c2Env <= 0) {
                this.c2Env = p2;
                if ((AR[0x17] & 8) != 0) {
                    if (this.c2Vol < 15) { this.c2Vol = this.c2Vol + 1; }
                } else {
                    if (this.c2Vol > 0) { this.c2Vol = this.c2Vol - 1; }
                }
            }
        }
        let p4: int = AR[0x21] & 7;
        if (this.c4On && p4 != 0) {
            this.c4Env = this.c4Env - 1;
            if (this.c4Env <= 0) {
                this.c4Env = p4;
                if ((AR[0x21] & 8) != 0) {
                    if (this.c4Vol < 15) { this.c4Vol = this.c4Vol + 1; }
                } else {
                    if (this.c4Vol > 0) { this.c4Vol = this.c4Vol - 1; }
                }
            }
        }
    }

    fsClock() => void {
        let st: int = this.fsStep;
        if ((st & 1) == 0) { this.lenClock(); }
        if (st == 2 || st == 6) { this.sweepClock(); }
        if (st == 7) { this.envClock(); }
        this.fsStep = (st + 1) & 7;
    }

    // Duty patterns as bit masks (bit i = output at step i): 12.5%, 25%, 50%, 75%.
    dutyMask(d: int) => int {
        let m: int = 0x80;
        if (d == 1) { m = 0x81; }
        if (d == 2) { m = 0xE1; }
        if (d == 3) { m = 0x7E; }
        return m;
    }

    // Called every 4 clocks. Steps the channels, mixes, and emits a sample about every 95 clocks.
    apuTick() => void {
        let l: int = 0;
        let r: int = 0;
        if (this.apuOn) {
            this.fsCnt = this.fsCnt + 4;
            if (this.fsCnt >= 8192) {
                this.fsCnt = this.fsCnt - 8192;
                this.fsClock();
            }
            let o1: int = 0;
            let o2: int = 0;
            let o3: int = 0;
            let o4: int = 0;
            if (this.c1On) {
                this.c1Tim = this.c1Tim - 4;
                while (this.c1Tim <= 0) {
                    this.c1Tim = this.c1Tim + (2048 - (AR[0x13] | ((AR[0x14] & 7) << 8))) * 4;
                    this.c1Pos = (this.c1Pos + 1) & 7;
                }
                if (((this.dutyMask((AR[0x11] >> 6) & 3) >> this.c1Pos) & 1) != 0) { o1 = this.c1Vol; }
            }
            if (this.c2On) {
                this.c2Tim = this.c2Tim - 4;
                while (this.c2Tim <= 0) {
                    this.c2Tim = this.c2Tim + (2048 - (AR[0x18] | ((AR[0x19] & 7) << 8))) * 4;
                    this.c2Pos = (this.c2Pos + 1) & 7;
                }
                if (((this.dutyMask((AR[0x16] >> 6) & 3) >> this.c2Pos) & 1) != 0) { o2 = this.c2Vol; }
            }
            if (this.c3On) {
                this.c3Tim = this.c3Tim - 4;
                while (this.c3Tim <= 0) {
                    this.c3Tim = this.c3Tim + (2048 - (AR[0x1D] | ((AR[0x1E] & 7) << 8))) * 2;
                    this.c3Pos = (this.c3Pos + 1) & 31;
                }
                let wb: int = IORAM[0x30 + (this.c3Pos >> 1)];
                let nib: int = wb >> 4;
                if ((this.c3Pos & 1) != 0) { nib = wb & 15; }
                let vs: int = (AR[0x1C] >> 5) & 3;
                if (vs != 0) { o3 = nib >> (vs - 1); }
            }
            if (this.c4On) {
                let np: int = this.noisePeriod();
                this.c4Tim = this.c4Tim - 4;
                while (this.c4Tim <= 0) {
                    this.c4Tim = this.c4Tim + np;
                    let x: int = (this.c4Lfsr ^ (this.c4Lfsr >> 1)) & 1;
                    this.c4Lfsr = (this.c4Lfsr >> 1) | (x << 14);
                    if ((AR[0x22] & 8) != 0) { this.c4Lfsr = (this.c4Lfsr & 0x7FBF) | (x << 6); }
                }
                if ((this.c4Lfsr & 1) == 0) { o4 = this.c4Vol; }
            }
            // DAC: a powered DAC maps 0..15 to -15..+15, an unpowered one is silent.
            let a1: int = 0;
            let a2: int = 0;
            let a3: int = 0;
            let a4: int = 0;
            if ((AR[0x12] & 0xF8) != 0) { a1 = o1 * 2 - 15; }
            if ((AR[0x17] & 0xF8) != 0) { a2 = o2 * 2 - 15; }
            if ((AR[0x1A] & 0x80) != 0) { a3 = o3 * 2 - 15; }
            if ((AR[0x21] & 0xF8) != 0) { a4 = o4 * 2 - 15; }
            let pan: int = AR[0x25];
            let ls: int = 0;
            let rs: int = 0;
            if ((pan & 0x10) != 0) { ls = ls + a1; }
            if ((pan & 0x20) != 0) { ls = ls + a2; }
            if ((pan & 0x40) != 0) { ls = ls + a3; }
            if ((pan & 0x80) != 0) { ls = ls + a4; }
            if ((pan & 0x01) != 0) { rs = rs + a1; }
            if ((pan & 0x02) != 0) { rs = rs + a2; }
            if ((pan & 0x04) != 0) { rs = rs + a3; }
            if ((pan & 0x08) != 0) { rs = rs + a4; }
            let vol: int = AR[0x24];
            l = ls * (((vol >> 4) & 7) + 1);
            r = rs * ((vol & 7) + 1);
        }
        this.sumL = this.sumL + l;
        this.sumR = this.sumR + r;
        this.sumN = this.sumN + 1;
        this.sAcc = this.sAcc + 176400;
        if (this.sAcc >= 4194304) {
            this.sAcc = this.sAcc - 4194304;
            this.emit();
        }
    }

    // Averages the ticks since the last sample, removes DC with a high-pass filter, appends to ABUF.
    emit() => void {
        let n: int = this.sumN;
        if (n < 1) { n = 1; }
        let inL: int = (this.sumL * 50) / n;
        let inR: int = (this.sumR * 50) / n;
        this.sumL = 0;
        this.sumR = 0;
        this.sumN = 0;
        let oL: int = inL - this.hpL;
        this.hpL = inL - (oL * 996) / 1000;
        let oR: int = inR - this.hpR;
        this.hpR = inR - (oR * 996) / 1000;
        if (oL > 32767) { oL = 32767; }
        if (oL < -32768) { oL = -32768; }
        if (oR > 32767) { oR = 32767; }
        if (oR < -32768) { oR = -32768; }
        if (this.aCnt < 4096) {
            ABUF[this.aCnt * 2] = oL as i16;
            ABUF[this.aCnt * 2 + 1] = oR as i16;
            this.aCnt = this.aCnt + 1;
        }
    }

    // ---------------------------------------------------------------- stepping
    // Timer input signal: TAC enable AND the selected bit of the 16-bit divider.
    tsig() => bool {
        let r: bool = false;
        let m: int = 0x200;
        if ((this.tac & 3) == 1) { m = 0x08; }
        if ((this.tac & 3) == 2) { m = 0x20; }
        if ((this.tac & 3) == 3) { m = 0x80; }
        if ((this.tac & 4) != 0 && (this.div16 & m) != 0) { r = true; }
        return r;
    }

    incTima() => void {
        this.tima = this.tima + 1;
        if (this.tima > 0xFF) {
            this.tima = this.tma;
            this.iff = this.iff | 0x04;
        }
    }

    // PPU mode from LY and the dot counter (LCD on): 2 OAM scan, 3 drawing, 0 HBlank, 1 VBlank.
    mode() => int {
        let m: int = 0;
        if (this.ly >= 144) {
            m = 1;
        } else if (this.dots < 80) {
            m = 2;
        } else if (this.dots < 252) {
            m = 3;
        }
        return m;
    }

    // Combined STAT interrupt line: IF bit 1 is raised on its rising edge.
    statUpdate() => void {
        let stat: int = IORAM[0x41];
        let lyc: int = IORAM[0x45];
        let md: int = this.mode();
        let line: bool = false;
        if ((stat & 0x08) != 0 && md == 0) { line = true; }
        if ((stat & 0x10) != 0 && md == 1) { line = true; }
        if ((stat & 0x20) != 0 && md == 2) { line = true; }
        if ((stat & 0x40) != 0 && this.ly == lyc) { line = true; }
        if (line && !this.statLine) { this.iff = this.iff | 0x02; }
        this.statLine = line;
    }

    // One scanline: background, window, then sprites. Shades 0..3 go to FB; BG colour indices to BGI.
    renderLine() => void {
        let y: int = this.ly;
        let lcdc: int = IORAM[0x40];
        let base: int = y * 160;
        let bgp: int = IORAM[0x47];
        let scx: int = IORAM[0x43];
        let scy: int = IORAM[0x42];
        let wy: int = IORAM[0x4A];
        let wx: int = IORAM[0x4B];
        if ((lcdc & 0x20) != 0 && y == wy) { this.wyHit = true; }
        let bgOn: bool = ((lcdc & 0x01) != 0);
        let winOn: bool = false;
        if (bgOn && (lcdc & 0x20) != 0 && this.wyHit && wx < 167) { winOn = true; }
        let drewWin: bool = false;
        for (let x: int = 0; x < 160; x = x + 1) {
            let idx: int = 0;
            let shade: int = 0;
            if (bgOn) {
                let px: int = 0;
                let py: int = 0;
                let map: int = 0x1800;
                if (winOn && (x + 7) >= wx) {
                    px = x + 7 - wx;
                    py = this.wl;
                    if ((lcdc & 0x40) != 0) { map = 0x1C00; }
                    drewWin = true;
                } else {
                    px = (x + scx) & 0xFF;
                    py = (y + scy) & 0xFF;
                    if ((lcdc & 0x08) != 0) { map = 0x1C00; }
                }
                let tn: int = VRAM[map + (py >> 3) * 32 + (px >> 3)];
                let taddr: int = 0;
                if ((lcdc & 0x10) != 0) {
                    taddr = tn * 16;
                } else {
                    if (tn > 127) { tn = tn - 256; }
                    taddr = 0x1000 + tn * 16;
                }
                let ro: int = taddr + (py & 7) * 2;
                let lo: int = VRAM[ro];
                let hi: int = VRAM[ro + 1];
                let bit: int = 7 - (px & 7);
                idx = (((hi >> bit) & 1) << 1) | ((lo >> bit) & 1);
                shade = (bgp >> (idx * 2)) & 3;
            }
            BGI[x] = idx as u8;
            FB[base + x] = shade as u8;
        }
        if (drewWin) { this.wl = this.wl + 1; }

        if ((lcdc & 0x02) != 0) {
            let sh: int = 8;
            if ((lcdc & 0x04) != 0) { sh = 16; }
            let n: int = 0;
            for (let i: int = 0; i < 40; i = i + 1) {
                let oy: int = OAM[i * 4] - 16;
                if (n < 10 && y >= oy && y < oy + sh) {
                    let ox: int = OAM[i * 4 + 1];
                    let pos: int = n;
                    while (pos > 0 && SPX[pos - 1] > ox) {
                        SPX[pos] = SPX[pos - 1];
                        SPI[pos] = SPI[pos - 1];
                        pos = pos - 1;
                    }
                    SPX[pos] = ox as u8;
                    SPI[pos] = i as u8;
                    n = n + 1;
                }
            }
            if (n > 0) {
                for (let x: int = 0; x < 160; x = x + 1) {
                    let found: bool = false;
                    for (let k: int = 0; k < n; k = k + 1) {
                        if (!found) {
                            let sxp: int = SPX[k] - 8;
                            if (x >= sxp && x < sxp + 8) {
                                let oi: int = SPI[k];
                                oi = oi * 4;
                                let flags: int = OAM[oi + 3];
                                let tile: int = OAM[oi + 2];
                                let row: int = y - (OAM[oi] - 16);
                                if ((flags & 0x40) != 0) { row = sh - 1 - row; }
                                if (sh == 16) { tile = (tile & 0xFE) + (row >> 3); }
                                row = row & 7;
                                let col: int = x - sxp;
                                if ((flags & 0x20) != 0) { col = 7 - col; }
                                let ro: int = tile * 16 + row * 2;
                                let lo: int = VRAM[ro];
                                let hi: int = VRAM[ro + 1];
                                let bit: int = 7 - col;
                                let idx: int = (((hi >> bit) & 1) << 1) | ((lo >> bit) & 1);
                                if (idx != 0) {
                                    found = true;
                                    let behind: bool = ((flags & 0x80) != 0);
                                    if (!(behind && BGI[x] != 0)) {
                                        let obp: int = IORAM[0x48];
                                        if ((flags & 0x10) != 0) { obp = IORAM[0x49]; }
                                        FB[base + x] = ((obp >> (idx * 2)) & 3) as u8;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Advance the PPU by 4 dots (LCD on).
    ppuTick() => void {
        this.dots = this.dots + 4;
        if (this.ly < 144 && !this.lineDrawn && this.dots >= 252) {
            this.renderLine();
            this.lineDrawn = true;
        }
        if (this.dots >= 456) {
            this.dots = this.dots - 456;
            this.ly = this.ly + 1;
            this.lineDrawn = false;
            if (this.ly == 144) {
                this.frameDone = true;
                this.iff = this.iff | 0x01;
            }
            if (this.ly >= 154) {
                this.ly = 0;
                this.wl = 0;
                this.wyHit = false;
            }
        }
        this.statUpdate();
    }

    advance(cyc: int) => void {
        let lcdOn: bool = ((IORAM[0x40] & 0x80) != 0);
        for (let k: int = 0; k < cyc; k = k + 4) {
            let old: bool = this.tsig();
            this.div16 = (this.div16 + 4) & 0xFFFF;
            if (old && !this.tsig()) { this.incTima(); }
            if (lcdOn) { this.ppuTick(); }
            this.apuTick();
        }
    }

    step() => int {
        let cyc: int = 4;
        let pend: int = this.ie & this.iff & 0x1F;
        if (this.halted && pend != 0) { this.halted = false; }
        if (this.ime && pend != 0) {
            let bit: int = 0;
            while ((pend & (1 << bit)) == 0) { bit = bit + 1; }
            this.iff = this.iff & (0xFF ^ (1 << bit));
            this.ime = false;
            this.imeCount = 0;
            if (this.haltBug) {
                // EI directly followed by HALT with an interrupt pending: the interrupt is taken, the HALT
                // bug is consumed and the return address is the HALT itself (it runs again after RETI).
                this.haltBug = false;
                this.push((this.pc - 1) & 0xFFFF);
            } else {
                this.push(this.pc);
            }
            this.pc = 0x40 + bit * 8;
            cyc = 20;
        } else if (this.halted) {
            cyc = 4;
        } else {
            cyc = this.execOne();
        }
        if (this.imeCount > 0) {
            this.imeCount = this.imeCount - 1;
            if (this.imeCount == 0) { this.ime = true; }
        }
        this.advance(cyc);
        return cyc;
    }

    willExecute() => bool {
        let pend: int = this.ie & this.iff & 0x1F;
        let r: bool = true;
        if (this.ime && pend != 0) { r = false; }
        if (this.halted && pend == 0) { r = false; }
        return r;
    }

    runFrame(budget: int) => int {
        let spent: int = 0;
        while (spent < budget) { spent = spent + this.step(); }
        return spent;
    }

    tickTimers() => void { }

    // ---------------------------------------------------------------- input / output
    setKey(idx: int, down: bool) => void {
        let m: int = 1 << idx;
        let sel: int = IORAM[0] & 0x30;
        if (down) {
            this.keys = this.keys | m;
            if (idx < 4 && (sel & 0x10) == 0) { this.iff = this.iff | 0x10; }
            if (idx >= 4 && (sel & 0x20) == 0) { this.iff = this.iff | 0x10; }
        } else {
            this.keys = this.keys & (0xFF ^ m);
        }
    }

    framebuffer() => u8* { return FB; }
    ramPtr() => u8* { return CRAM; }
    width() => int { return 160; }
    height() => int { return 144; }
    soundActive() => bool { return this.apuOn; }
    audioCount() => int { return this.aCnt; }
    audioPtr() => i16* { return ABUF; }
    audioClear() => void { this.aCnt = 0; }
    serialText() => char* { return SERIAL; }
    title() => char* { return TITLE; }
    serialCount() => int { return this.serN; }
    setDoctor(on: bool) => void { this.doctor = on; }

    hex2(out: Str.String*, v: int) => void {
        out.appendChar(HEXD[(v >> 4) & 0x0F]);
        out.appendChar(HEXD[v & 0x0F]);
    }

    hex4(out: Str.String*, v: int) => void {
        this.hex2(out, (v >> 8) & 0xFF);
        this.hex2(out, v & 0xFF);
    }

    /// Appends one Gameboy Doctor line (with trailing newline): state before the next instruction.
    trace(out: Str.String*) => void {
        out.append("A:");
        this.hex2(out, this.a);
        out.append(" F:");
        this.hex2(out, this.f);
        out.append(" B:");
        this.hex2(out, this.b);
        out.append(" C:");
        this.hex2(out, this.c);
        out.append(" D:");
        this.hex2(out, this.d);
        out.append(" E:");
        this.hex2(out, this.e);
        out.append(" H:");
        this.hex2(out, this.h);
        out.append(" L:");
        this.hex2(out, this.l);
        out.append(" SP:");
        this.hex4(out, this.sp);
        out.append(" PC:");
        this.hex4(out, this.pc);
        out.append(" PCMEM:");
        this.hex2(out, this.rd(this.pc));
        out.append(",");
        this.hex2(out, this.rd(this.pc + 1));
        out.append(",");
        this.hex2(out, this.rd(this.pc + 2));
        out.append(",");
        this.hex2(out, this.rd(this.pc + 3));
        out.append("\n");
    }
}
