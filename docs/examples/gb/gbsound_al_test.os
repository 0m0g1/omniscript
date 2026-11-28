// docs/examples/gb/gbsound_al_test.os
// One-off check that PCM streaming works: plays a 440 Hz square wave for 3 seconds.
// Run: .\bin\Release-windows-x86_64\Osengine.exe docs/examples/gb/gbsound_al_test.os
module Console = import "standard/1/Console.os";
module Time = import "standard/1/Time.os";
module S = import "gbsound_al.os";

let BUF: i16[2048];

function main() => int {
    let out: S.PcmOut;
    if (!out.open(44100)) { Console.println("pcm stream failed"); return 2; }

    let phase: int = 0;
    let total: int = 0;
    while (total < 44100 * 3) {
        if (out.space() >= 1024) {
            for (let i: int = 0; i < 1024; i = i + 1) {
                let v: int = 6000;
                if ((phase / 50) % 2 == 1) { v = -6000; }
                phase = phase + 1;
                BUF[i * 2] = v as i16;
                BUF[i * 2 + 1] = v as i16;
            }
            out.push(BUF, 1024);
            total = total + 1024;
        } else {
            Time.sleep(5 as u64);
        }
    }
    Time.sleep(400 as u64);
    out.close();
    Console.println("done");
    return 0;
}
